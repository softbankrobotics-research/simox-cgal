#ifndef __SKELETON_GRASP_PLANNER_H__
#define __SKELETON_GRASP_PLANNER_H__

#include <GraspPlanning/GraspStudio.h>
#include <GraspPlanning/GraspPlanner/GraspPlanner.h>
#include <GraspPlanning/GraspQuality/GraspQualityMeasureWrenchSpace.h>
#include <VirtualRobot/Grasping/GraspSet.h>
#include "ApproachMovementSkeleton.h"
#include "SimoxCGAL.h"
#include "SegmentedObject.h"

    /*!
    *
    *
    * A general grasp planning class that utilizes ApprachMovementGenerator for generating grasp hypothesis and
    * GraspQualityMeasure to score them.
    *
    */
class SkeletonGraspPlanner : public GraspStudio::GraspPlanner
{
public:

    /*!
            Constructor
            \param graspSet All planned grasps are added to this GraspSet.
            \param graspQuality The quality of generated grasps is evaluated with this object
            \param approach Approach movements are generated by this object.
            \param minQuality The quality that must be achieved at minimum by the GraspQualityMesurement module
            \param forceClosure When true, only force closure grasps are generated.
        */
    SkeletonGraspPlanner(VirtualRobot::GraspSetPtr graspSet, GraspStudio::GraspQualityMeasurePtr graspQuality, ApproachMovementSkeletonPtr approach, float minQuality = 0.0f, bool forceClosure = true);

    // destructor
    virtual ~SkeletonGraspPlanner();

    /*!
            Creates new grasps.
            \param nrGrasps The number of grasps to be planned.
            \param timeOutMS The time out in milliseconds. Planning is stopped when this time is exceeded. Disabled when zero.
            \return Number of generated grasps.
        */
    virtual int plan(int nrGrasps, int timeOutMS = 0, VirtualRobot::SceneObjectSetPtr obstacles = VirtualRobot::SceneObjectSetPtr());


protected:

    bool timeout();

    VirtualRobot::GraspPtr planGrasp(VirtualRobot::SceneObjectSetPtr obstacles = VirtualRobot::SceneObjectSetPtr());

    VirtualRobot::SceneObjectPtr object;
    VirtualRobot::EndEffectorPtr eef;
    std::string preshape;

//    clock_t startTime;
//    int timeOutMS;
    VirtualRobot::EndEffector::ContactInfoVector contacts;
    GraspStudio::GraspQualityMeasurePtr graspQuality;
    ApproachMovementSkeletonPtr approach;

    float minQuality;
    bool forceClosure;
};

typedef boost::shared_ptr<SkeletonGraspPlanner> SkeletonGraspPlannerPtr;

#endif /* __GENERIC_GRASP_PLANNER_H__ */
