// Ardumower Sunray 
// Copyright (c) 2013-2020 by Alexander Grau, Grau GmbH
// Licensed GPLv3 for open source use
// or Grau GmbH Commercial License for commercial use (http://grauonline.de/cms2/?page_id=153)

#include "op.h"
#include <Arduino.h>
#include "../../robot.h"
#include "../../StateEstimator.h"
#include "../../LineTracker.h"
#include "../../Stats.h"
#include "../../map.h"
#include "../../events.h"


MowOp::MowOp(){
    lastMapRoutingFailed = false;
    mapRoutingFailedCounter = 0;
    gotoNearTargetSince = 0;
    gotoDone = false;
}

String MowOp::name(){
    return "Mow";
}

void MowOp::begin(){
    // goto mode (AT+R) - skip pathfinding, drive directly to free point
    if (maps.gotoActive){
        maps.gotoActive = false;
        gotoNearTargetSince = 0;
        gotoDone = false;
        CONSOLE.print("OP_MOW (goto mode)");
        CONSOLE.print(" mowPWMCurr=");
        CONSOLE.print(motor.motorMowPWMCurr);
        CONSOLE.print(" saved=");
        CONSOLE.println(maps.savedMowMotorRunningBeforeGoto);
        motor.enableTractionMotors(true);
        motor.setReleaseBrakesWhenZero(false);
        motor.setLinearAngularSpeed(0,0);
        if (maps.savedMowMotorRunningBeforeGoto){
            CONSOLE.println("goto: mow ON (was ON before)");
            motor.enableMowMotor = true;
            motor.setMowState(true);
        } else {
            CONSOLE.println("goto: mow OFF (was OFF before)");
            motor.enableMowMotor = false;
            motor.stopImmediately(true);
        }
        battery.setIsDocked(false);
        timetable.setMowingCompletedInCurrentTimeFrame(false);
        stateEstimator.lastFixTime = millis();
        maps.setLastTargetPoint(stateEstimator.stateX, stateEstimator.stateY);
        lastMapRoutingFailed = false;
        mapRoutingFailedCounter = 0;
        return;
    }

    // Resuming existing free points (GoTo/pathfinder) after interruption (e.g. GPS loss)
    if (maps.wayMode == WAY_FREE && maps.freePoints.numPoints > 0) {
        CONSOLE.println("OP_MOW (resume free path)");
        gotoNearTargetSince = 0;
        gotoDone = false;
        motor.enableTractionMotors(true);
        motor.setReleaseBrakesWhenZero(false);
        motor.setLinearAngularSpeed(0, 0);
        battery.setIsDocked(false);
        timetable.setMowingCompletedInCurrentTimeFrame(false);
        stateEstimator.lastFixTime = millis();
        maps.setLastTargetPoint(stateEstimator.stateX, stateEstimator.stateY);
        return;
    }

    bool error = false;
    bool routingFailed = false;      

    CONSOLE.println("OP_MOW");      
    motor.enableTractionMotors(true); // allow traction motors to operate         
    motor.setReleaseBrakesWhenZero(false);
    motor.setLinearAngularSpeed(0,0);      
    if (((previousOp != &escapeReverseOp) && (previousOp != &escapeForwardOp)) || (DISABLE_MOW_MOTOR_AT_OBSTACLE))  motor.setMowState(false);              
    battery.setIsDocked(false);                
    timetable.setMowingCompletedInCurrentTimeFrame(false); 
    CONSOLE.println(" Relais will be set to true (on)");
#ifdef DRV_CAN_ROBOT
    relaisDriver.setRelaisState(RELAIS_1_NODE_ID, true); // enable relais 1 (Lidar) when not charging               
#endif

    // plan route to next target point 

    dockOp.dockReasonRainTriggered = false;    

    if (((initiatedByOperator) && (previousOp == &idleOp)) || (lastMapRoutingFailed))  maps.clearObstacles();

    if (maps.startMowing(stateEstimator.stateX, stateEstimator.stateY)){
        if (maps.nextPoint(true, stateEstimator.stateX, stateEstimator.stateY)) {
            stateEstimator.lastFixTime = millis();                
            maps.setLastTargetPoint(stateEstimator.stateX, stateEstimator.stateY);        
            //stateSensor = SENS_NONE;
            motor.setMowState(true);                
        } else {
            error = true;
            CONSOLE.println("error: no waypoints!");
            //op = stateOp;                
        }
    } else error = true;

    if (error){
        stateEstimator.stateSensor = SENS_MAP_NO_ROUTE;
        //op = OP_ERROR;
        routingFailed = true;
        motor.setMowState(false);
    }

    if (routingFailed){
        lastMapRoutingFailed = true; 
        mapRoutingFailedCounter++;    
        if (mapRoutingFailedCounter > 60){
            CONSOLE.println("error: too many map routing errors!");
            stateEstimator.stateSensor = SENS_MAP_NO_ROUTE;
            Logger.event(EVT_ERROR_NO_MAP_ROUTE_GIVEUP);
            changeOp(errorOp);      
        } else {    
            Logger.event(EVT_ERROR_NO_MAP_ROUTE);
            changeOp(gpsRebootRecoveryOp, true);
        }
    } else {
        lastMapRoutingFailed = false;
        mapRoutingFailedCounter = 0;
    }
}


void MowOp::end(){
}

void MowOp::run(){
    if (!detectObstacle()){
        detectObstacleRotation();                              
    }        
    // line tracking
    lineTracker.trackLine(true); 
    detectSensorMalfunction();    
    battery.resetIdle();

    // GoTo waypoint advance / target detection
    // GPS imprecision may prevent TARGET_REACHED_TOLERANCE (0.1m) from firing
    if (maps.wayMode == WAY_FREE) {
        float dist = maps.distanceToTargetPoint(stateEstimator.stateX, stateEstimator.stateY);
        if (dist >= 0.5) {
            gotoNearTargetSince = 0;
        } else if (dist > 0.1) {
            // near waypoint but trackLine couldn't reach it → start fallback timer
            if (gotoNearTargetSince == 0) gotoNearTargetSince = millis();
            if (millis() - gotoNearTargetSince > 8000) {
                if (maps.freePointsIdx + 1 < maps.freePoints.numPoints) {
                    CONSOLE.println("goto: advance waypoint (GPS timeout)");
                    maps.nextPoint(false, stateEstimator.stateX, stateEstimator.stateY);
                    gotoNearTargetSince = 0;
                } else {
                    CONSOLE.println("goto: target reached (GPS timeout)");
                    onTargetReached();
                    return;
                }
            }
        }
    }
    
    if (timetable.shouldAutostopNow()){
        if (DOCKING_STATION){
            CONSOLE.println("TIMETABLE - DOCKING");
            dockOp.setInitiatedByOperator(false);
            changeOp(dockOp);
        } else {
            CONSOLE.println("TIMETABLE - IDLE");
            changeOp(idleOp);
        }
    }
}

void MowOp::onRainTriggered(){
    if (DOCKING_STATION){
        CONSOLE.println("RAIN TRIGGERED");
        Logger.event(EVT_RAIN_DOCKING);
        stateEstimator.stateSensor = SENS_RAIN;
        dockOp.dockReasonRainTriggered = true;
        #ifdef DRV_SIM_ROBOT
            dockOp.dockReasonRainAutoStartTime = millis() + 60000 * 3; // try again after 3 minutes 
        #else
            dockOp.dockReasonRainAutoStartTime = millis() + 60000 * 60; // try again after one hour 
        #endif
        dockOp.setInitiatedByOperator(false);
        changeOp(dockOp);              
    }
}

void MowOp::onTempOutOfRangeTriggered(){
    if (DOCKING_STATION){
        CONSOLE.println("TEMP OUT-OF-RANGE TRIGGERED");
        Logger.event(EVT_TEMPERATURE_OUT_OF_RANGE_DOCK);
        stateEstimator.stateSensor = SENS_TEMP_OUT_OF_RANGE;
        dockOp.dockReasonRainTriggered = true;
        dockOp.dockReasonRainAutoStartTime = millis() + 60000 * 60; // try again after one hour      
        dockOp.setInitiatedByOperator(false);
        changeOp(dockOp);              
    }
}

void MowOp::onBatteryLowShouldDock(){    
    CONSOLE.println("BATTERY LOW TRIGGERED - DOCKING");
    Logger.event(EVT_BATTERY_LOW_DOCK);    
    if (DOCKING_STATION){
        dockOp.setInitiatedByOperator(false);
        changeOp(dockOp);    
    } else {
        idleOp.setInitiatedByOperator(false);
        stateEstimator.stateSensor = SENS_BAT_UNDERVOLTAGE;
        changeOp(idleOp);    
    }            
}

void MowOp::onTimetableStopMowing(){        
}

void MowOp::onTimetableStartMowing(){        
}

void MowOp::onObstacle(){
    if ((!DOCK_DETECT_OBSTACLE_IN_DOCK) && (maps.isBetweenLastAndNextToLastDockPoint())) {
      //CONSOLE.println("triggerObstacle: ignoring, because in dock");      
      return;
    }
    CONSOLE.println("triggerObstacle");      
    stats.statMowObstacles++;      
    if (maps.isDocking()) {    
        if (maps.retryDocking(stateEstimator.stateX, stateEstimator.stateY)) {
            changeOp(escapeReverseOp, true);                      
            return;
        }
    } 
    if ((OBSTACLE_AVOIDANCE) && (maps.wayMode != WAY_DOCK)){    
        changeOp(escapeReverseOp, true);      
    } else {     
        stateEstimator.stateSensor = SENS_OBSTACLE;
        CONSOLE.println("error: obstacle!");            
        changeOp(errorOp);                
    }
}
    
void MowOp::onObstacleRotation(){
    CONSOLE.println("triggerObstacleRotation");    
    stats.statMowObstacles++;   
    if ((OBSTACLE_AVOIDANCE) && (maps.wayMode != WAY_DOCK)){    
        if (FREEWHEEL_IS_AT_BACKSIDE){    
            changeOp(escapeForwardOp, true);      
        } else {
            changeOp(escapeReverseOp, true);
        }
    } else { 
        stateEstimator.stateSensor = SENS_OBSTACLE;
        CONSOLE.println("error: obstacle!");            
        changeOp(errorOp);
    }
}


void MowOp::onOdometryError(){
    if (ENABLE_ODOMETRY_ERROR_DETECTION){
        CONSOLE.println("error: odometry error!");    
        stateEstimator.stateSensor = SENS_ODOMETRY_ERROR;
        Logger.event(EVT_ERROR_ODOMETRY);
        changeOp(errorOp);
    }
}
    
void MowOp::onMotorOverload(){
  if (ENABLE_OVERLOAD_DETECTION){
    if (motor.motorOverloadDuration > 20000){
        CONSOLE.println("error: motor overload!");    
        stateEstimator.stateSensor = SENS_OVERLOAD;
        Logger.event(EVT_ERROR_MOTOR_OVERLOAD);
        changeOp(errorOp);
        return;
    }
  }
}
    
void MowOp::onMotorError(){
    if (ENABLE_FAULT_OBSTACLE_AVOIDANCE){
        if (motor.motorError){
            // this is the molehole situation: motor error will permanently trigger on molehole => we try obstacle avoidance (molehole avoidance strategy)
            motor.motorError = false; // reset motor error flag
            stateEstimator.motorErrorCounter++;
            CONSOLE.print("MowOp::onMotorError motorErrorCounter=");       
            CONSOLE.println(stateEstimator.motorErrorCounter);
            if (maps.wayMode != WAY_DOCK){
                if (stateEstimator.motorErrorCounter < FAULT_MAX_SUCCESSIVE_ALLOWED_COUNT){                     
            //stateEstimator.stateSensor = SENS_MOTOR_ERROR;
                    Logger.event(EVT_ERROR_MOTOR_ERROR);            
                    changeOp(escapeReverseOp, true);     // trigger obstacle avoidance 
                    return;
                }
            }
            // obstacle avoidance failed with too many motor errors (it was probably not a molehole situation)
            CONSOLE.println("error: motor error - giving up!");
            stateEstimator.motorErrorCounter = 0;
            stateEstimator.stateSensor = SENS_MOTOR_ERROR;
            Logger.event(EVT_ERROR_MOTOR_ERROR_GIVEUP);
            changeOp(errorOp);
            return;      
        }  
    } else {
        CONSOLE.println("no obstacle avoidance activated on motor errors, giving up");    
        stateEstimator.stateSensor = SENS_MOTOR_ERROR;
        Logger.event(EVT_ERROR_MOTOR_ERROR_GIVEUP);
        changeOp(errorOp);        
        return;
    }
}

void MowOp::onTargetReached(){
    if (maps.wayMode == WAY_MOW){    
        maps.clearObstacles(); // clear obstacles if target reached
        stateEstimator.motorErrorCounter = 0; // reset motor error counter if target reached
        stateEstimator.stateSensor = SENS_NONE; // clear last triggered sensor
    } else if (maps.wayMode == WAY_FREE) {
        // Only stop on final waypoint; intermediate perimeter waypoints just pass through
        if (maps.freePointsIdx + 1 >= maps.freePoints.numPoints) {
            CONSOLE.println("goto: target reached");
            gotoDone = true;
            changeOp(idleOp);
        } else {
            CONSOLE.println("goto: waypoint reached");
        }
    }
}


void MowOp::onGpsFixTimeout(){
    // no gps solution
    if (REQUIRE_VALID_GPS){
#ifdef UNDOCK_IGNORE_GPS_DISTANCE
        if (!maps.isUndocking() || getDockDistance() > UNDOCK_IGNORE_GPS_DISTANCE){
#else
        if (!maps.isUndocking()){
#endif
            stateEstimator.stateSensor = SENS_GPS_FIX_TIMEOUT;            
            changeOp(gpsWaitFixOp, true);
        }
    }
}

void MowOp::onGpsNoSignal(){
    if (REQUIRE_VALID_GPS){
#ifdef UNDOCK_IGNORE_GPS_DISTANCE
        if (!maps.isUndocking() || getDockDistance() > UNDOCK_IGNORE_GPS_DISTANCE){
#else
        if (!maps.isUndocking()){
#endif
            stateEstimator.stateSensor = SENS_GPS_INVALID;            
            changeOp(gpsWaitFloatOp, true);
        }
    }
}

void MowOp::onKidnapped(bool state){
    if (state){
        stateEstimator.stateSensor = SENS_KIDNAPPED;      
        motor.setLinearAngularSpeed(0,0, false); 
        motor.setMowState(false);            
        changeOp(kidnapWaitOp, true); 
    }
}

void MowOp::onNoFurtherWaypoints(){
    // GoTo completion is already handled in onTargetReached() → changeOp(idleOp)
    // Prevent this from overriding to dockOp
    if (gotoDone){
        gotoDone = false;
        CONSOLE.println("goto: done (onNoFurtherWaypoints)");
        return;
    }
    CONSOLE.println("mowing finished!");
    Logger.event(EVT_MOWING_COMPLETED);
    timetable.setMowingCompletedInCurrentTimeFrame(true);
    if (!stateEstimator.finishAndRestart){             
        if (DOCKING_STATION && stateEstimator.dockAfterFinish){
            dockOp.setInitiatedByOperator(false);
            changeOp(dockOp);               
        } else {
            idleOp.setInitiatedByOperator(false);
            changeOp(idleOp); 
        }
    }
}

void MowOp::onImuTilt(){
    stateEstimator.stateSensor = SENS_IMU_TILT;
    Logger.event(EVT_ROBOT_TILTED);
    changeOp(errorOp);
}

void MowOp::onImuError(){
    stateEstimator.stateSensor = SENS_IMU_TIMEOUT;
    Logger.event(EVT_ERROR_IMU_TIMEOUT);
    changeOp(errorOp);
}
