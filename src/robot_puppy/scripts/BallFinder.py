#!/usr/bin/env python3

import rclpy
import cv2
import numpy as np
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data

from cv_bridge import CvBridge, CvBridgeError
from sensor_msgs.msg import Image, LaserScan
from assn4.msg import BallLocation


class Robot(Node):
    def __init__(self):
        super().__init__('ballfinder')
        self.bridge = CvBridge()
        self.raw_image = []
        self.ranges = []
        self.ball_pub = self.create_publisher(BallLocation, '/ball_location', 10) #(type, 'name', queue_size)
        # You should modify the image from the robot's camera so that you can
        # understand what your code is doing. Mask out everything but the
        # yellow pixels. Turn all the yellow pixels chartreuse. Whatever you
        # want. Throw a colored vertical line in that shows what your pixel
        # average is.
        self.im_publisher = self.create_publisher(Image, '/ball_image', 10) #publish Image to ball_image topic
        self.timer = self.create_timer(0.1, self.main_loop)
        self.create_subscription( #get data from image_raw topic, which is the camera feed from the robot
            Image,
            '/oakd/rgb/preview/image_raw',
            self.handle_image,
            qos_profile_sensor_data,
        )
        self.create_subscription( #get data from scan topic, which is the laser scan data from the robot
            LaserScan,
            '/scan',
            self.handle_scan,
            qos_profile_sensor_data,
        )

    def handle_image(self, msg):
        try:
            self.raw_image = self.bridge.imgmsg_to_cv2(msg, 'bgr8') #convert data raw_image data from image_raw topic (?)
        except CvBridgeError:
            print("Unable to convert ROS image to OpenCV format.")

    def handle_scan(self, msg): #grab laser scan data from scan topic and store in self.ranges
        self.ranges = msg.ranges
    
    def ball_targeted(self, bearing_offset, distance, threshold_ratio=0.05):
        """Return True when the ball is centered within a small offset and the distance is valid."""
        if bearing_offset is None:
            return False
        if not np.isfinite(distance) or distance <= 0.0:
            return False
        max_offset = max(10, int(self.raw_image.shape[1] * threshold_ratio))
        return abs(bearing_offset) <= max_offset

    def main_loop(self):
        if len(self.raw_image) == 0 or len(self.ranges) == 0: #if nothing received, dont run.
            return

        image = self.raw_image.copy()
        height, width = image.shape[:2]
        hsv = cv2.cvtColor(image, cv2.COLOR_BGR2HSV) #convert image to hsv for filtering
        lower_yellow = np.array([20, 100, 100]) #set bounds for yellow
        upper_yellow = np.array([30, 255, 255])
        mask = cv2.inRange(hsv, lower_yellow, upper_yellow) #create mask for yellow pixels

        yellow_cols = np.where(mask == 255)[1] #get column indices of yellow pixels
        ball_location = BallLocation()
        ball_location.targeted = False
        bearing_offset = None
        if len(yellow_cols) == 0:
            ball_location.bearing = -1
            ball_location.distance = -1.0
            avg_x = width // 2
        else:
            avg_x = int(np.mean(yellow_cols)) #calculate average column index of yellow pixels
            bearing_offset = int(avg_x - (width / 2))
            ball_location.bearing = bearing_offset #use center offset as bearing
            scan_index = int(avg_x * len(self.ranges) / width)
            scan_index = max(0, min(scan_index, len(self.ranges) - 1))
            distance = self.ranges[scan_index]
            if np.isfinite(distance) and distance > 0.0:
                ball_location.distance = float(distance)
            else:
                ball_location.distance = -1.0
            ball_location.targeted = self.ball_targeted(bearing_offset, ball_location.distance)

        image[mask > 0] = [0, 255, 0] #color yellow pixels
        cv2.line(image, (avg_x, 0), (avg_x, image.shape[0]), (255, 0, 0), 2)

        try:
            self.im_publisher.publish(self.bridge.cv2_to_imgmsg(image, 'bgr8'))
        except CvBridgeError:
            print('Unable to convert mask to ROS image')
        self.ball_pub.publish(ball_location)


def main(args=None):
    rclpy.init(args=args)
    robot = Robot()
    rclpy.spin(robot)
    robot.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()