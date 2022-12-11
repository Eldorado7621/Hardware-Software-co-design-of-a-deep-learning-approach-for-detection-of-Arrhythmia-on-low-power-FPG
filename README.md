# Hardware-Software-co-design-of-a-deep-learning-approach-for-detection-of-Arrhythmia-on-low-power-FPGA

# Overview
This project is on hardware/software co-design of a deep learning approach for fast detection of Arrhythmia on low power and low resource embedded FPGA device. The main goal of the project is to improve the rate of low power embedded devices to detect abnormalties in heart rate.

# Motivation and Description

According to WHO, cardiovascular diseases are the world leading cause of death in the world-claiming about 17.9 million lives every year. [1](https://www.who.int/health-topics/cardiovascular-diseases#tab=tab_1)
As such being able to detect abnormalities early enough before it becomes extremely dangerous have the potential of saving a lot of lives. 

Taking advantage of deep learning and tiny ML, I was able to achieve a 500% increase in computation time at low power. The approach used involves a hardware/ software co-design: I designed a Continuous Wavelet Transform IP that converts the 1D time series ECG signal (MIT BIH dataset) to 2D spectogram which serves as input data to the 2D CNN model that was deployed on the embedded device. For the 2D Convolution, I used a Hybrid float-6 2D CNN hardware accelerator that was designed by [Yarib Nevarez et al](https://github.com/YaribNevarez/sensor-edge/blob/main/sensor-edge-journal/main/main.pdf). 
