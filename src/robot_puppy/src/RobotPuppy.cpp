#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "robot_puppy/msg/ball_location.hpp"

class RobotPuppyNode : public rclcpp::Node {
public:
    // Constructor
    RobotPuppyNode() : Node("robot_puppy"), current_state_(State::SEARCH) { //this node's called "robot_puppy"

        cmd_vel_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("cmd_vel", 10); //publisher for velocity
        ball_sub_ = this->create_subscription<robot_puppy::msg::BallLocation>( //subscriber to ball location topic ball_finder
            "ball_location", 10,
            [this](const robot_puppy::msg::BallLocation::SharedPtr msg) { //
                this->ball_bearing_ = msg->bearing; //"bearing in this object = bearing in msg"
                this->ball_distance_ = msg->distance; //same with distance
                this->ball_found_ = msg->found; //same with found
            });
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(100),
            std::bind(&RobotPuppyNode::control_loop, this));
    }

private:
    // Tunable parameters - adjust these values without recompiling
    struct TunableParams {
        double search_rotation_speed = 0.5;  // rad/s - how fast to spin when searching
        double approach_distance_goal = 1.0; // meters - distance to stop at
        double kick_speed = 0.3;             // m/s - forward speed when kicking
        int kick_duration_ms = 3000;         // milliseconds - how long to kick
    } params_;

    enum class State { SEARCH, APPROACH, KICK };
    State current_state_;

    double ball_bearing_ = 0.0; // hold targeting data
    double ball_distance_ = 0.0;
    bool ball_found_ = false;
    int bearing_goal = 125;
    // distance_goal moved to TunableParams as approach_distance_goal

    pid_controller bearing_pid_; // PID controller for bearing
    pid_controller distance_pid_; // PID controller for distance

    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_; //velo pub
    rclcpp::Subscription<robot_puppy::msg::BallLocation>::SharedPtr ball_sub_; //ball lo sub
    rclcpp::TimerBase::SharedPtr timer_; //timer for control loop
    rclcpp::Time kick_start_time_; //time point for kick timing

    void control_loop() {
        geometry_msgs::msg::Twist twist_cmd; // pub'd twist msg as twist_cmd

        switch (current_state_) { //switch for FSM
            case State::SEARCH: 
                twist_cmd.linear.x = 0.0; //ensure no forward movement
                twist_cmd.angular.z = params_.search_rotation_speed; //rotate in place
                if(ball_found_) {
                    twist_cmd.angular.z = 0.0; //stop rotating
                    current_state_ = State::APPROACH; //switch to approach state when ball found
                }
                break;

            case State::APPROACH:
                double dt = 0.1; // Assuming control loop runs every 100ms
                // PID compute takes ERROR (desired - actual), not raw sensor value
                bearing_pid_.compute(bearing_goal - ball_bearing_, dt); //angular PID call
                distance_pid_.compute(params_.approach_distance_goal - ball_distance_, dt); //linear PID call

                break;

            case State::KICK:
                // todo: Drive forward at 0.3 m/s. 
                // Handle the 3-second timing logic here, then switch to SEARCH.
                break;
        }

        // Broadcast the motor command to the bus
        cmd_vel_pub_->publish(twist_cmd);
    }
};

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    // Spin acts as the infinite loop, keeping the node alive to listen for callbacks
    rclcpp::spin(std::make_shared<RobotPuppyNode>());
    rclcpp::shutdown();
    return 0;
}