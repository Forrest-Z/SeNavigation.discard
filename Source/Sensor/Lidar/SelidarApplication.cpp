/*
 * SelidarApplication.cpp
 *
 *  Created on: 2016年10月12日
 *      Author: lichq
 */

#include "SelidarApplication.h"
#include <Console/Console.h>
//for debugging
#include <assert.h>
#include <Time/Utils.h>

namespace NS_Selidar
{
  
#define DEG2RAD(x) ((x)*M_PI/180.)
#define RAD2DEG(x) ((x)*180./M_PI)
  
#ifndef _countof
#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#endif
  
  SelidarApplication::SelidarApplication ()
  {
    serial_baudrate = 115200;
    frame_id = "laser_frame";
    scan_count = 0;
    scan_timeout = 3;
  }
  
  SelidarApplication::~SelidarApplication ()
  {
    scan_count = 0;
  }
  
  void
  SelidarApplication::loadParameters ()
  {
    parameter.loadConfigurationFile ("selidar.xml");
    serial_port = parameter.getParameter ("serial_port", "/dev/ttyUSB0");
    serial_baudrate = parameter.getParameter ("serial_baudrate", 115200);
    frame_id = parameter.getParameter ("frame_id", "laser_frame");
  }
  
#ifdef DUPLEX_MODE
  bool
  SelidarApplication::checkSelidarHealth (SelidarDriver * drv)
  { 
    int op_result;
    SelidarHealth healthinfo;

    op_result = drv->getHealth (healthinfo);

    if (IS_OK(op_result))
    { 
      NS_NaviCommon::console.debug ("Selidar health status : %d, errcode: %d",
          healthinfo.status, healthinfo.err_code);

      if (healthinfo.status != StatusFine)
      { 
        NS_NaviCommon::console.warning ("Selidar's status is not fine! ");
        return false;
      }
      else
      { 
        NS_NaviCommon::console.message ("Selidar's status is not fine! ");
        return true;
      }

    }
    else
    { 
      return false;
    }
  }

  bool
  SelidarApplication::checkSelidarInfo (SelidarDriver * drv)
  { 
    int op_result;
    SelidarInfo device_info;

    op_result = drv->getDeviceInfo (device_info);

    if (IS_OK(op_result))
    { 
      NS_NaviCommon::console.debug ("Selidar device info :");
      NS_NaviCommon::console.debug ("\t model : %d ", device_info.model);
      NS_NaviCommon::console.debug ("\t hw ver : %d ", device_info.hw_id);
      NS_NaviCommon::console.debug ("\t fw ver : %d.%d ", device_info.fw_major,
          device_info.fw_minor);
      return true;
    }
    else
    { 
      return false;
    }

  }

  bool
  SelidarApplication::stopScanService (NS_ServiceType::RequestBase* request,
      NS_ServiceType::ResponseBase* response)
  { 
    if (!drv.isConnected ())
    return false;

    drv.stop ();

    return true;
  }

  bool
  SelidarApplication::startScanService (NS_ServiceType::RequestBase* request,
      NS_ServiceType::ResponseBase* response)
  { 
    if (!drv.isConnected ())
    return false;

    NS_NaviCommon::console.message ("Start motor");

    drv.startScan ();
    return true;
  }
#endif
  
  void
  SelidarApplication::publishScan (SelidarMeasurementNode *nodes,
                                   size_t node_count, NS_NaviCommon::Time start,
                                   double scan_time, float angle_min,
                                   float angle_max)
  {
    NS_DataType::LaserScan* scan_msg = new NS_DataType::LaserScan;
    
    scan_msg->header.stamp = start;
    scan_msg->header.frame_id = frame_id;

    scan_count_lock.lock ();
    scan_count++;
    if (scan_count == 1)
    {
      NS_NaviCommon::console.debug ("Got first scan data!");
      got_first_scan_cond.notify_one ();
    }
    scan_count_lock.unlock ();

    scan_msg->angle_min = angle_min;
    scan_msg->angle_max = angle_max;

    scan_msg->angle_increment = (scan_msg->angle_max - scan_msg->angle_min)
        / (double) (node_count - 1);
    
    scan_msg->scan_time = scan_time;
    scan_msg->time_increment = scan_time / (double) (node_count - 1);
    scan_msg->range_min = 0.15f;
    scan_msg->range_max = 6.0f;
    
    scan_msg->intensities.resize (node_count);
    scan_msg->ranges.resize (node_count);

    for (size_t i = 0; i < node_count; i++)
    {
      float read_value = (float) nodes[i].distance_scale_1000 / 1000.0f;
      if (read_value == 0.0)
        scan_msg->ranges[i] = std::numeric_limits<float>::infinity ();
      else scan_msg->ranges[i] = read_value;
    }
    
    dispitcher->publish (NS_NaviCommon::DATA_TYPE_LASER_SCAN, scan_msg);
  }
  
  void
  SelidarApplication::scanLoop ()
  {
    int op_result;
    drv.startScan ();
    NS_NaviCommon::Time start_scan_time;
    NS_NaviCommon::Time end_scan_time;
    double scan_duration;
    while (running)
    {
      SelidarMeasurementNode nodes[360 * 2];
      size_t count = _countof(nodes);
      start_scan_time = NS_NaviCommon::Time::now ();
      op_result = drv.grabScanData (nodes, count);
      end_scan_time = NS_NaviCommon::Time::now ();
      scan_duration = (end_scan_time - start_scan_time).toSec () * 1e-3;
      if (op_result == Success)
      {
        float angle_min = DEG2RAD(0.0f);
        float angle_max = DEG2RAD(359.0f);
        
        const int angle_compensate_nodes_count = 360;
        const int angle_compensate_multiple = 1;
        int angle_compensate_offset = 0;
        SelidarMeasurementNode angle_compensate_nodes[angle_compensate_nodes_count];
        memset (angle_compensate_nodes, 0, angle_compensate_nodes_count * sizeof(SelidarMeasurementNode));

        for (int i = 0; i < count; i++)
        {
          if (nodes[i].distance_scale_1000 != 0)
          {
            float angle = (float) (nodes[i].angle_scale_100) / 100.0f;
            int angle_value = (int) (angle * angle_compensate_multiple);
            if ((angle_value - angle_compensate_offset) < 0)
              angle_compensate_offset = angle_value;

            for (int j = 0; j < angle_compensate_multiple; j++)
            {
              angle_compensate_nodes[angle_value - angle_compensate_offset + j] = nodes[i];
            }
          }
        }
        publishScan (angle_compensate_nodes, angle_compensate_nodes_count,
                     start_scan_time, scan_duration, angle_min, angle_max);
        
      }
    }
  }
  
  void
  SelidarApplication::initialize ()
  {
    NS_NaviCommon::console.message ("selidar is initializing!");
    
    loadParameters ();
    
    // make connection...
    if (IS_FAIL(
        drv.connect (serial_port.c_str (), (unsigned int )serial_baudrate, 0)))
    {
      NS_NaviCommon::console.error (
          "cannot bind to the specified serial port %s.", serial_port.c_str ());
    }
    
#ifdef DUPLEX_MODE
    // reset lidar
    drv.reset ();
    NS_NaviCommon::delay (5000);

    // check health...
    if (!checkSelidarHealth (&drv))
    { 
      return;
    }

    NS_NaviCommon::delay (100);

    // get device info...
    if (!checkSelidarInfo (&drv))
    { 
      return;
    }
    NS_NaviCommon::delay (100);

    service->advertise (
        NS_NaviCommon::SERVICE_TYPE_STOP_SCAN,
        boost::bind (&SelidarApplication::stopScanService, this, _1, _2));
    service->advertise (
        NS_NaviCommon::SERVICE_TYPE_START_SCAN,
        boost::bind (&SelidarApplication::startScanService, this, _1, _2));
#endif
    
    NS_NaviCommon::delay (100);
    
    initialized = true;
    
    NS_NaviCommon::console.message ("selidar has initialized!");
  }
  
  void
  SelidarApplication::run ()
  {
    NS_NaviCommon::console.message ("selidar is running!");
    
    running = true;
    
    scan_thread = boost::thread (
        boost::bind (&SelidarApplication::scanLoop, this));

    int wait_times = 0;
    scan_count_lock.lock ();
    while (wait_times++ <= scan_timeout && scan_count == 0)
    {
      got_first_scan_cond.timed_wait (scan_count_lock,
                                      (boost::get_system_time () + boost::posix_time::seconds (1)));
    }
    scan_count_lock.unlock ();

    if (scan_count == 0)
    {
      NS_NaviCommon::console.error ("Can't got first scan from LIDAR.");
      running = false;
    }

  }
  
  void
  SelidarApplication::quit ()
  {
    NS_NaviCommon::console.message ("selidar is quitting!");
    
#ifdef DUPLEX_MODE
    drv.stop ();
#endif
    
    running = false;
    
    scan_thread.join ();
    
    drv.disconnect ();
  }

}

