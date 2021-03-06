/*
 * Application.cpp
 *
 *  Created on: 2016年9月29日
 *      Author: seeing
 */

#include "Application.h"
#include <Console/Console.h>
#include <Time/Time.h>
#include <Parameter/Parameter.h>

static NS_NaviCommon::Dispitcher global_dispitcher;
static NS_NaviCommon::Service global_service;

Application::Application ()
{
  dispitcher = &global_dispitcher;
  service = &global_service;
}

Application::~Application ()
{
  dispitcher = NULL;
  service = NULL;
}

void
Application::globalInitialize ()
{
  NS_NaviCommon::Parameter para;
  para.loadConfigurationFile ("application.xml");
  
  if (para.getParameter ("disable_stdout", 0) == 1)
  {
    NS_NaviCommon::disableStdoutStream ();
    NS_NaviCommon::console.warning ("Stdout stream has been disabled!");
  }
  
  if (para.getParameter ("debug", 1) == 1)
  {
    NS_NaviCommon::console.debugOn ();
  }
  else
  {
    NS_NaviCommon::console.debugOff ();
  }


  NS_NaviCommon::Time::init ();
  
  global_dispitcher.initialize ();
  global_service.initialize ();
}

void
Application::initialize ()
{
  NS_NaviCommon::console.warning ("initializing with base application class!");
}

void
Application::run ()
{
  NS_NaviCommon::console.warning ("running with base application class!");
}

void
Application::quit ()
{
  NS_NaviCommon::console.warning ("quitting with base application class!");
}
