// Ardumower Sunray 
// Copyright (c) 2013-2020 by Alexander Grau, Grau GmbH
// Licensed GPLv3 for open source use
// or Grau GmbH Commercial License for commercial use (http://grauonline.de/cms2/?page_id=153)

#include "op.h"
#include <Arduino.h>
#include "../../robot.h"
#include "../../StateEstimator.h"
#include "../../map.h"
#include "../../config.h"



String EscapeReverseOp::name(){
    return "EscapeReverse";
}

void EscapeReverseOp::begin(){
    // obstacle avoidance
    driveReverseStopTime = millis() + 3000;
    // avoidSide is set by the caller before changeOp, reset after use
}


void EscapeReverseOp::end(){
    // reset avoidSide so next escape is neutral by default
    avoidSide = 0;
}


void EscapeReverseOp::run(){
    battery.resetIdle();
    // side-avoidance: reverse + slight turn away from detected side
    // avoidSide: -1 = obstacle left (turn right), +1 = obstacle right (turn left), 0 = straight back
    float angular = 0;
    #if SONAR_SIDE_AVOIDANCE_ENABLED
    if (avoidSide != 0){
        angular = -avoidSide * SONAR_SIDE_AVOID_STRENGTH;  // turn away from obstacle
    }
    #endif
    motor.setLinearAngularSpeed(-0.1, angular);
    if (DISABLE_MOW_MOTOR_AT_OBSTACLE)  motor.setMowState(false);

    if (millis() > driveReverseStopTime){
        CONSOLE.println("driveReverseStopTime");
        motor.stopImmediately(false);
        driveReverseStopTime = 0;
        if (detectLift()) {
            CONSOLE.println("error: lift sensor!");
            stateEstimator.stateSensor = SENS_LIFT;
            changeOp(errorOp);
            return;
        }
        if (maps.isDocking()){
            CONSOLE.println("continue docking");
            // continue without obstacles
            changeOp(*nextOp, false);    // continue current operation
        } else {
            CONSOLE.println("continue operation with virtual obstacle");
            #if SONAR_OFFSET_OBSTACLE_ENABLED
            if (avoidSide != 0){
                // place the virtual obstacle on the side where it was detected (avoidSide -1 = left, +1 = right),
                // so the pathfinder routes around it on the free side.
                // perpAngle points to the LEFT of the heading, hence the negated avoidSide.
                float heading = stateEstimator.stateDelta;  // current heading in radians
                float perpAngle = heading + PI/2.0f;        // perpendicular direction
                float offsetX = stateEstimator.stateX - avoidSide * SONAR_OFFSET_OBSTACLE_DIST * cos(perpAngle);
                float offsetY = stateEstimator.stateY - avoidSide * SONAR_OFFSET_OBSTACLE_DIST * sin(perpAngle);
                maps.addObstacle(offsetX, offsetY);
            } else {
                maps.addObstacle(stateEstimator.stateX, stateEstimator.stateY);
            }
            #else
            maps.addObstacle(stateEstimator.stateX, stateEstimator.stateY);
            #endif
            maps.requestTangentialPerimeterRecovery();
            changeOp(*nextOp, false);    // continue current operation
        }
    }
}



void EscapeReverseOp::onImuTilt(){
    stateEstimator.stateSensor = SENS_IMU_TILT;
    changeOp(errorOp);
}

void EscapeReverseOp::onImuError(){
    stateEstimator.stateSensor = SENS_IMU_TIMEOUT;
    changeOp(errorOp);
}
