#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "assn4/msg/ball_location.hpp"
// TODO: Include your custom BallLocation message header here

class RobotPuppyNode : public rclcpp::Node {
public:
    // Constructor
    RobotPuppyNode() : Node("robot_puppy"), current_state_(State::SEARCH) { //this node's called "robot_puppy"

        cmd_vel_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("cmd_vel", 10); //publisher for velocity
        ball_sub_ = this->create_subscription<assn4::msg::BallLocation>( //subscriber to ball location topic ball_finder
            "ball_location", 10,
            [this](const assn4::msg::BallLocation::SharedPtr msg) { //
                this->ball_bearing_ = msg->bearing; //"bearing in this object = bearing in msg"
                this->ball_distance_ = msg->distance; //same with distance
                this->ball_found_ = msg->found; //same with found
            });
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(100),
            std::bind(&RobotPuppyNode::control_loop, this));
    }

private:

    enum class State { SEARCH, APPROACH, KICK };
    State current_state_;

    double ball_bearing_ = 0.0; //where ball_location data gets assigned
    double ball_distance_ = 0.0;
    bool ball_targeted_ = false;
    
    // --- ROS 2 Interfaces ---
    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_; //publisher for velocity commands
    rclcpp::Subscription<your_package::msg::BallLocation>::SharedPtr ball_sub_; //subscriber for ball location data
    rclcpp::TimerBase::SharedPtr timer_; //timer for control loop

    // --- The Main Logic ---
    void control_loop() {
        geometry_msgs::msg::Twist twist_cmd; //twist_cmd name of message of type Twist, which is the message type for velocity commands

        switch (current_state_) { //switch for FSM
            case State::SEARCH: 
                twist_cmd.linear.x = 0.0; //ensure no forward movement
                twist_cmd.angular.z = 0.5; //rotate in place
                
                if(ball_targeted_) {
                    twist_cmd.linear.z = 0.0; //stop rotating, may not be necessary gotta check that
                    current_state_ = State::APPROACH; //switch to approach state when ball found
                }
                break;

            case State::APPROACH:
                // TODO: Calculate PID values using ball_bearing_ and ball_distance_.
                // Assign results to twist_cmd.linear.x and twist_cmd.angular.z.
                // If errors are small enough, change current_state_ to KICK.
                break;

            case State::KICK:
                // TODO: Drive forward at 0.3 m/s. 
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