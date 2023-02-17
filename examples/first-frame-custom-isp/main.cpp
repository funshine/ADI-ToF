/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2019, Analog Devices, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <aditof/camera.h>
#include <aditof/depth_sensor_interface.h>
#include <aditof/frame.h>
#include <aditof/system.h>
#include <aditof/version.h>
#include <fstream>
#include <glog/logging.h>
#include <ios>
#include <iostream>
#include <chrono>
#include <thread>

using namespace aditof;

Status save_frame(aditof::Frame &frame, std::string frameType) {

    uint16_t *data1;
    FrameDataDetails fDetails;
    Status status = Status::OK;

    status = frame.getData(frameType, &data1);
    if (status != Status::OK) {
        LOG(ERROR) << "Could not get frame data " + frameType + "!";
        return status;
    }

    if (!data1) {
        LOG(ERROR) << "no memory allocated in frame";
        return status;
    }

    std::ofstream g("out_" + frameType + "_" + fDetails.type + ".bin",
                    std::ios::binary);
    frame.getDataDetails(frameType, fDetails);
    g.write((char *)data1, fDetails.width * fDetails.height * sizeof(uint16_t));
    g.close();

    return status;
}

int main(int argc, char *argv[]) {

    google::InitGoogleLogging(argv[0]);
    FLAGS_alsologtostderr = 1;

    LOG(INFO) << "SDK version: " << aditof::getApiVersion()
              << " | branch: " << aditof::getBranchVersion()
              << " | commit: " << aditof::getCommitVersion();

    Status status = Status::OK;

    // if (argc < 2) {
    //     LOG(ERROR) << "No config file provided! ./first-frame-custom-isp <config_file>";
    //     return 0;
    // }

    // std::string configFile = argv[1];

    System system;

    std::vector<std::shared_ptr<Camera>> cameras;
    system.getCameraList(cameras);
    if (cameras.empty()) {
        LOG(WARNING) << "No cameras found";
        return 0;
    }

    auto camera = cameras.front();

    // status = camera->setControl("initialization_config", configFile);
    // if (status != Status::OK) {
    //     LOG(ERROR) << "Failed to set control!";
    //     return 0;
    // }

    status = camera->initialize();
    if (status != Status::OK) {
        LOG(ERROR) << "Could not initialize camera!";
        return 0;
    }

    // aditof::CameraDetails cameraDetails;
    // camera->getDetails(cameraDetails);

    // LOG(INFO) << "SD card image version: " << cameraDetails.sdCardImageVersion;
    // LOG(INFO) << "Kernel version: " << cameraDetails.kernelVersion;
    // LOG(INFO) << "U-Boot version: " << cameraDetails.uBootVersion;

    std::vector<std::string> frameTypes;
    camera->getAvailableFrameTypes(frameTypes);
    if (frameTypes.empty()) {
        LOG(ERROR) << "no frame type avaialble!";
        return 0;
    }

    status = camera->setFrameType("vga");
    if (status != Status::OK) {
        LOG(ERROR) << "Could not set camera frame type!";
        return 0;
    }

    auto sensor = camera->getSensor();
    if (!sensor) {
        LOG(ERROR) << "Could not get camera depth sensor!";
        return 0;
    }

    std::vector<std::string> controls;
    sensor->getAvailableControls(controls);
    if (controls.empty()) {
        LOG(INFO) << "No controls avaialble!";
    } else {
        std::string controlName = "fps";
        std::string controlValue;
        LOG(INFO) << "Get avaialble controls: ";
        for (auto ct : controls) {
            status = sensor->getControl(ct, controlValue);
            if (status != Status::OK) {
                LOG(ERROR) << "Could not get sensor control: " << ct << "!";
            } else {
                LOG(INFO) << "Get control " << ct << " : " << controlValue;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                auto val = stoi(controlValue) + 1;
                if (val < 0) val = 0;
                auto newVal = std::to_string(val);
                status = sensor->setControl(ct, newVal);
                if (status != Status::OK) {
                    LOG(ERROR) << "Could not set sensor control: " << ct
                               << " - " << newVal << "!";
                } else {
                    LOG(INFO) << "Set control " << ct << " : " << newVal;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        for (auto ct : controls) {
            status = sensor->getControl(ct, controlValue);
            if (status != Status::OK) {
                LOG(ERROR) << "Could not get sensor control: " << ct << "!";
            } else {
                LOG(INFO) << "Get control " << ct << " : " << controlValue;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    status = camera->start();
    if (status != Status::OK) {
        LOG(ERROR) << "Could not start the camera!";
        return 0;
    }
    aditof::Frame frame;

    status = camera->requestFrame(&frame);
    if (status != Status::OK) {
        LOG(ERROR) << "Could not request frame!";
        return 0;
    } else {
        LOG(INFO) << "succesfully requested frame!";
    }

    save_frame(frame, "ir");
    save_frame(frame, "depth");

    // Example of reading temperature from hardware
    uint16_t sensorTmp = 0;
    uint16_t laserTmp = 0;
    status = camera->adsd3500GetSensorTemperature(sensorTmp);
    if (status != Status::OK) {
        LOG(ERROR) << "Could not read sensor temperature!";
    }

    status = camera->adsd3500GetLaserTemperature(laserTmp);
    if (status != Status::OK) {
        LOG(ERROR) << "Could not read laser temperature!";
    }

    LOG(INFO) << "Sensor temperature: " << sensorTmp;
    LOG(INFO) << "Laser temperature: " << laserTmp;

    return 0;
}
