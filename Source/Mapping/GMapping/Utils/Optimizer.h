#ifndef _OPTIMIZER_H_
#define _OPTIMIZER_H_

#include "../Utils/Point.h"

namespace NS_GMapping
{
  
  using namespace std;
  
  struct OptimizerParams
  {
    double discretization;
    double angularStep, linearStep;
    int iterations;
    double maxRange;
  };
  
  template<typename Likelihood, typename Map>
    struct Optimizer
    {
      Optimizer (const OptimizerParams& params);
      OptimizerParams params;
      Map lmap;
      Likelihood likelihood;
      OrientedPoint
      gradientDescent (const RangeReading& oldReading,
                       const RangeReading& newReading);
      OrientedPoint
      gradientDescent (const RangeReading& oldReading,
                       const OrientedPoint& pose, OLocalMap& Map);
      enum Move
      {
        Forward, Backward, Left, Right, TurnRight, TurnLeft
      };
    };
  
  template<typename Likelihood, typename Map>
    Optimizer<Likelihood, Map>::Optimizer (const OptimizerParams& p)
        : params (p), lmap (p.discretization)
    {
    }
  
  template<typename Likelihood, typename Map>
    OrientedPoint
    Optimizer<Likelihood, Map>::gradientDescent (const RangeReading& oldReading,
                                                 const RangeReading& newReading)
    {
      lmap.clear ();
      lmap.update (oldReading, OrientedPoint (0, 0, 0), params.maxRange);
      OrientedPoint delta = absoluteDifference (newReading.getPose (),
                                                oldReading.getPose ());
      OrientedPoint bestPose = delta;
      double bestScore = likelihood (lmap, newReading, bestPose,
                                     params.maxRange);
      int it = 0;
      double lstep = params.linearStep, astep = params.angularStep;
      bool increase;
      /*	cout << "bestScore=" << bestScore << endl;;*/
      do
      {
        increase = false;
        OrientedPoint itBestPose = bestPose;
        double itBestScore = bestScore;
        bool itIncrease;
        do
        {
          itIncrease = false;
          OrientedPoint testBestPose = itBestPose;
          double testBestScore = itBestScore;
          for (Move move = Forward; move <= TurnLeft;
              move = (Move) ((int) move + 1))
          {
            OrientedPoint testPose = itBestPose;
            switch (move)
            {
              case Forward:
                testPose.x += lstep;
                break;
              case Backward:
                testPose.x -= lstep;
                break;
              case Left:
                testPose.y += lstep;
                break;
              case Right:
                testPose.y -= lstep;
                break;
              case TurnRight:
                testPose.theta -= astep;
                break;
              case TurnLeft:
                testPose.theta += astep;
                break;
            }
            double score = likelihood (lmap, newReading, testPose,
                                       params.maxRange);
            if (score > testBestScore)
            {
              testBestScore = score;
              testBestPose = testPose;
            }
          }
          if (testBestScore > itBestScore)
          {
            itBestScore = testBestScore;
            itBestPose = testBestPose;
            /*				cout << "s=" << itBestScore << " ";*/
            itIncrease = true;
          }
        }
        while (itIncrease);
        if (itBestScore > bestScore)
        {
          /*			cout << "S(" << itBestScore << "," <<  bestScore<< ")";*/
          bestScore = itBestScore;
          bestPose = itBestPose;
          increase = true;
        }
        else
        {
          it++;
          lstep *= 0.5;
          astep *= 0.5;
        }
      }
      while (it < params.iterations);
      /*	cout << "FinalBestScore" << bestScore << endl;*/
      cout << endl;
      return bestPose;
    }
  
  template<typename Likelihood, typename Map>
    OrientedPoint
    Optimizer<Likelihood, Map>::gradientDescent (const RangeReading& reading,
                                                 const OrientedPoint& pose,
                                                 OLocalMap& lmap)
    {
      OrientedPoint bestPose = pose;
      double bestScore = likelihood (lmap, reading, bestPose, params.maxRange);
      int it = 0;
      double lstep = params.linearStep, astep = params.angularStep;
      bool increase;
      /*	cout << "bestScore=" << bestScore << endl;;*/
      do
      {
        increase = false;
        OrientedPoint itBestPose = bestPose;
        double itBestScore = bestScore;
        bool itIncrease;
        do
        {
          itIncrease = false;
          OrientedPoint testBestPose = itBestPose;
          double testBestScore = itBestScore;
          for (Move move = Forward; move <= TurnLeft;
              move = (Move) ((int) move + 1))
          {
            OrientedPoint testPose = itBestPose;
            switch (move)
            {
              case Forward:
                testPose.x += lstep;
                break;
              case Backward:
                testPose.x -= lstep;
                break;
              case Left:
                testPose.y += lstep;
                break;
              case Right:
                testPose.y -= lstep;
                break;
              case TurnRight:
                testPose.theta -= astep;
                break;
              case TurnLeft:
                testPose.theta += astep;
                break;
            }
            double score = likelihood (lmap, reading, testPose,
                                       params.maxRange);
            if (score > testBestScore)
            {
              testBestScore = score;
              testBestPose = testPose;
            }
          }
          if (testBestScore > itBestScore)
          {
            itBestScore = testBestScore;
            itBestPose = testBestPose;
            /*				cout << "s=" << itBestScore << " ";*/
            itIncrease = true;
          }
        }
        while (itIncrease);
        if (itBestScore > bestScore)
        {
          /*			cout << "S(" << itBestScore << "," <<  bestScore<< ")";*/
          bestScore = itBestScore;
          bestPose = itBestPose;
          increase = true;
        }
        else
        {
          it++;
          lstep *= 0.5;
          astep *= 0.5;
        }
      }
      while (it < params.iterations);
      /*	cout << "FinalBestScore" << bestScore << endl;*/
      cout << endl;
      return bestPose;
    }

} // end namespace NS_GMapping
#endif
