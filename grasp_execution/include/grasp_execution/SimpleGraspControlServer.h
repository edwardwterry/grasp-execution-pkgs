/**
 */
#ifndef GRASP_EXECUTION_SIMPLEGRASPCONTROLSERVER_H
#define GRASP_EXECUTION_SIMPLEGRASPCONTROLSERVER_H

#include <ros/ros.h>
#include <actionlib/server/simple_action_server.h>
#include <actionlib/client/simple_action_client.h>

#include <grasp_execution_msgs/GraspControlAction.h>
#include <sensor_msgs/JointState.h>

#include <architecture_binding/Thread.h>
#include <arm_components_name_manager/ArmComponentsNameManager.h>
#include <arm_components_name_manager/ArmJointStateSubscriber.h>


namespace grasp_execution
{

/**
 * Accepts grasp_execution_msgs/GraspControl action and publishes the goal joint state to a
 * sensor_msgs::JointState joint control topic, in order to close/open the hand.
 * The grasp is considered finished when the grippers are at the goal position, or if they
 * haven't moved for a certain time.
 *
 * \author Jennifer Buehler
 * \date March 2016
 */
class SimpleGraspControlServer{
	protected:
	typedef actionlib::ActionServer<grasp_execution_msgs::GraspControlAction> GraspControlActionServerT;
	typedef GraspControlActionServerT::GoalHandle GoalHandle;
	
	public:
	/**
     * \param gripper_joint_names names of the joints which are involved in the grasping action. These are the joints which are
     *      checked against the goal state - or if they are not moving for a while, are considered to be grapsing (see \e noMoveTolerance)
	 * \param goalTolerance when the grippers are this close (in rad) to their target, we assume the grasp as finished.
	 * \param noMoveTolerance if a gripper does not move this amount within checkStateFreq, it is considered to have met resistance (touched object).
	 */
	SimpleGraspControlServer(
		ros::NodeHandle &n, 
		std::string& action_topic_name,
		std::string& joint_states_topic,
		std::string& joint_control_topic,
        const arm_components_name_manager::ArmComponentsNameManager& _joints_manager,
		float goalTolerance,
		float noMoveTolerance,
		float checkStateFreq);

	~SimpleGraspControlServer();

	/**
	 * Starts action server and does internal initialisation.
	 */
	bool init();

	/**
 	 * methods to be executed for shutting down the action server
	 */
	void shutdown();

	bool executingGoal();
	bool hasCurrentGoal();
	
	private: 
    typedef architecture_binding::unique_lock<architecture_binding::mutex>::type unique_lock;	
	
	/**
	 * abort grippers movement execution;
	 */
	void abortExecution();

	/** 
	 * set the flag that execution of the current trajectory has finished in the implementation.
 	 */
	void setExecutionFinished(bool flag, bool success);
	/**
	 * returns flag indicating if execution of trajectory has finished in implementation.
	 */
	bool executionFinished(bool& success);

	/**
	 * Receive a new goal
	 */
	void actionCallback(GoalHandle& goal);
	/**
	 * Receive a cancel trajectory instruction
	 */
	void cancelCallback(GoalHandle& goal);
	
    /**
     * Checks whether the grippers are not moving and updates \e no_move_stat
     * accordingly.
     * Precondition: move_stat and no_move_stat must be initialized to the
     * size of the number of gripper joints, with all 0 values.
     * \retval -1  error doing the check
     * \retval 0 check successful, but goal conditions not reached
     * \retval 1 check successful and goal conditions reached
     */
    int updateGrippersCheck();
    /**
     * Loop for calling updateGrippersCheck at a rate of \e updateRate
     */
    static void updateGrippersCheckLoop(SimpleGraspControlServer * _this, float updateRate);

    void cancelGripperCheckThread();

    /*
	 *  tolerance (in radian) at which a goal pose of the grippers is considered reached
     */
	float GOAL_TOLERANCE;

    /*
     *  As soon as a gripper joint moves less than this amount of rads 
     *  since the last update of its position, it
	 *  is considered to not have moved. This is checked at a rate of
     *  \e gripper_angles_check_freq.
     */
	float NO_MOVE_TOLERANCE;
        
    std::vector<std::string> gripper_joint_names;

    /*
	 * last gripper state that was saved. Gripper states are observed at a rate of gripper_angles_check_freq.
     */
	std::vector<float> last_gripper_angles;

    /*
	 *  time stamp of last_gripper_angles
     */
	ros::Time time_last_gripper_angles;
	
    /*
	 *  rate at which the gripper states are checked
     */
	float gripper_angles_check_freq;

    /*
     *  thread to run the loop updateGrippersCheckLoop() at the rate of
     *  gripper_angles_check_freq
     */
    architecture_binding::thread * gripper_check_thread;

    /*
	 *  number of times the grippers have not moved since the last check (since last_gripper_angles)
     *  these are given in the same order as gripper_joint_names.
     *  This field is updated by updateGrippersCheck which is called at rate gripper_angles_check_freq.
     */
    std::vector<int> no_move_stat;
    
    /*
     *  number of times that movement was recorded for grippers since the goal was accepted.
     *  This field is updated by updateGrippersCheck which is called at rate gripper_angles_check_freq.
     */
    std::vector<int> move_stat;
	
    /*
	 * structure that subsribes to joint state messages and keeps the Jaco specific structures
     */
    const arm_components_name_manager::ArmComponentsNameManager joints_manager;

    /*
     * Subscribes to joint states in order to always have the most recent information.
     */
    arm_components_name_manager::ArmJointStateSubscriber joint_state_subscriber;    

    /*
	 *  target gripper joints state. Protected
     *  by goal_lock.
     */
	std::vector<float> target_gripper_angles;

    /*
     *  to lock access to execution_finished, current_goal,
     *  target_gripper_angles and has_goal 
     */
	architecture_binding::mutex goal_lock;
	
	GoalHandle current_goal;
	bool has_goal;
	bool execution_finished;
	bool execution_successful;

	ros::Publisher joint_control_pub;

	GraspControlActionServerT * action_server;

	bool initialized;

};
}  // namespace

#endif // GRASP_EXECUTION_SIMPLEGRASPCONTROLSERVER_H
