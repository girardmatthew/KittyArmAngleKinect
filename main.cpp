// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING 1
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS 1

#include <array>
#include <iostream>
#include <map>
#include <vector>
#include <k4arecord/playback.h>
#include <k4a/k4a.h>
#include <k4abt.h>

#include <BodyTrackingHelpers.h>
#include <Utilities.h>
#include <Window3dWrapper.h>

//Azure Speech Services
#include <fstream>
#include <string>
#include <speechapi_cxx.h>

//Azure Speech Services
using namespace std;
using namespace Microsoft::CognitiveServices::Speech;
using namespace Microsoft::CognitiveServices::Speech::Audio;

//End Azure Speech Services

//Declar Structures
struct InputSettings;

//Declare Functions
void synthesizeSpeech(string str);
string recognizeSpeech();
void PlayFromDevice(InputSettings inputSettings);

//Global Constants
#define track_arm 1
#define track_hand 2
#define track_face 3
#define track_body 4
#define turn_off 5


int feature = track_body; //track body by default

void PrintUsage()
{
    printf("\nUSAGE: (k4abt_)simple_3d_viewer.exe SensorMode[NFOV_UNBINNED, WFOV_BINNED](optional) RuntimeMode[CPU](optional)\n");
    printf("  - SensorMode: \n");
    printf("      NFOV_UNBINNED (default) - Narrow Field of View Unbinned Mode [Resolution: 640x576; FOI: 75 degree x 65 degree]\n");
    printf("      WFOV_BINNED             - Wide Field of View Binned Mode [Resolution: 512x512; FOI: 120 degree x 120 degree]\n");
    printf("  - RuntimeMode: \n");
    printf("      CPU - Use the CPU only mode. It runs on machines without a GPU but it will be much slower\n");
    printf("      OFFLINE - Play a specified file. Does not require Kinect device\n");
    printf("e.g.   (k4abt_)simple_3d_viewer.exe WFOV_BINNED CPU\n");
    printf("e.g.   (k4abt_)simple_3d_viewer.exe CPU\n");
    printf("e.g.   (k4abt_)simple_3d_viewer.exe WFOV_BINNED\n");
    printf("e.g.   (k4abt_)simple_3d_viewer.exe OFFLINE MyFile.mkv\n");
}

void PrintAppUsage()
{
    printf("\n");
    printf(" Basic Navigation:\n\n");
    printf(" Rotate: Rotate the camera by moving the mouse while holding mouse left button\n");
    printf(" Pan: Translate the scene by holding Ctrl key and drag the scene with mouse left button\n");
    printf(" Zoom in/out: Move closer/farther away from the scene center by scrolling the mouse scroll wheel\n");
    printf(" Select Center: Center the scene based on a detected joint by right clicking the joint with mouse\n");
    printf("\n");
    printf(" Key Shortcuts\n\n");
    printf(" ESC: quit\n");
    printf(" h: help\n");
    printf(" b: body visualization mode\n");
    printf(" k: 3d window layout\n");
    printf("\n");
}

// Global State and Key Process Function
bool s_isRunning = true;
Visualization::Layout3d s_layoutMode = Visualization::Layout3d::OnlyMainView;
bool s_visualizeJointFrame = false;

float dot(float a[3], float b[3])  //calculates dot product of a and b
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

float mag(float a[3])  //calculates magnitude of a
{
    return std::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
}

struct InputSettings
{
    k4a_depth_mode_t DepthCameraMode = K4A_DEPTH_MODE_NFOV_UNBINNED;
    bool CpuOnlyMode = false;
    bool Offline = false;
    std::string FileName;
};

void voiceMenu()
{
    InputSettings inputSettings;

    string str = "switch";
    synthesizeSpeech("Yes father?");
    bool caseFound = false;

    int64_t voice_cmd = 0;

    string track_arm_chk = "track arm";
    string track_hand_chk = "track hand";
    string track_face_chk = "track face";
    string track_body_chk = "track body";
    string turn_off_chk = "go back to sleep";

    do
    {
        string str = recognizeSpeech();

        if (str.find(track_arm_chk) != std::string::npos) {
            std::cout << "Tracking arm..." << '\n';
            voice_cmd = track_arm;
        }
        else if (str.find(track_hand_chk) != std::string::npos) {
            std::cout << "Tracking hand..." << '\n';
            voice_cmd = track_hand;
        }
        else if (str.find(track_face_chk) != std::string::npos) {
            std::cout << "Tracking face..." << '\n';
            voice_cmd = track_face;
        }
        else if (str.find(track_body_chk) != std::string::npos) {
            std::cout << "Tracking body..." << '\n';
            voice_cmd = track_body;
        }
        else if (str.find(turn_off_chk) != std::string::npos) {
            std::cout << "Turning off..." << '\n';
            voice_cmd = turn_off;
        }
        else {
            cout << "Unknown command: " << str << std::endl;
        }

        switch (voice_cmd)
        {
        case track_arm:
            synthesizeSpeech("Sure. Starting arm tracking.");
            feature = track_arm;
            break;
        case track_hand:
            synthesizeSpeech("Yes. Initializing hand tracking.");
            feature = track_hand;
            break;
        case track_face:
            synthesizeSpeech("Yes. Initializing face tracking.");
            feature = track_face;
            break;
        case track_body:
            synthesizeSpeech("Okay. Tracking body.");
            feature = track_body;
            break;
        case turn_off: // Quit
            synthesizeSpeech("I'm sleepy. Goodnight father.");
            s_isRunning = false;
            break;
        }
    } while (voice_cmd == 0);
}

int64_t ProcessKey(void* /*context*/, int key)
{
    // https://www.glfw.org/docs/latest/group__keys.html
    switch (key)
    {
        // Quit
    case GLFW_KEY_ESCAPE:
        s_isRunning = false;
        break;
    case GLFW_KEY_K:
        s_layoutMode = (Visualization::Layout3d)(((int)s_layoutMode + 1) % (int)Visualization::Layout3d::Count);
        break;
    case GLFW_KEY_B:
        s_visualizeJointFrame = !s_visualizeJointFrame;
        break;
    case GLFW_KEY_H:
        PrintAppUsage();
        break;
    case GLFW_KEY_V:
        voiceMenu();
        break;
    }
    return 1;
}

int64_t CloseCallback(void* /*context*/)
{
    s_isRunning = false;
    return 1;
}

bool ParseInputSettingsFromArg(int argc, char** argv, InputSettings& inputSettings)
{
    for (int i = 1; i < argc; i++)
    {
        std::string inputArg(argv[i]);
        if (inputArg == std::string("NFOV_UNBINNED"))
        {
            inputSettings.DepthCameraMode = K4A_DEPTH_MODE_NFOV_UNBINNED;
        }
        else if (inputArg == std::string("WFOV_BINNED"))
        {
            inputSettings.DepthCameraMode = K4A_DEPTH_MODE_WFOV_2X2BINNED;
        }
        else if (inputArg == std::string("CPU"))
        {
            inputSettings.CpuOnlyMode = true;
        }
        else if (inputArg == std::string("OFFLINE"))
        {
            inputSettings.Offline = true;
            if (i < argc - 1) {
                // Take the next argument after OFFLINE as file name
                inputSettings.FileName = argv[i + 1];
                i++;
            }
            else {
                return false;
            }
        }
        else
        {
            printf("Error command not understood: %s\n", inputArg.c_str());
            return false;
        }
    }
    return true;

}

void VisualizeResult(k4abt_frame_t bodyFrame, Window3dWrapper& window3d, int depthWidth, int depthHeight) {

    // Obtain original capture that generates the body tracking result
    k4a_capture_t originalCapture = k4abt_frame_get_capture(bodyFrame);
    k4a_image_t depthImage = k4a_capture_get_depth_image(originalCapture);

    std::vector<Color> pointCloudColors(depthWidth * depthHeight, { 1.f, 1.f, 1.f, 1.f });

    // Read body index map and assign colors
    k4a_image_t bodyIndexMap = k4abt_frame_get_body_index_map(bodyFrame);
    const uint8_t* bodyIndexMapBuffer = k4a_image_get_buffer(bodyIndexMap);
    for (int i = 0; i < depthWidth * depthHeight; i++)
    {
        uint8_t bodyIndex = bodyIndexMapBuffer[i];
        if (bodyIndex != K4ABT_BODY_INDEX_MAP_BACKGROUND)
        {
            uint32_t bodyId = k4abt_frame_get_body_id(bodyFrame, bodyIndex);
            pointCloudColors[i] = g_bodyColors[bodyId % g_bodyColors.size()];
        }
    }
    k4a_image_release(bodyIndexMap);

    // Visualize point cloud
    window3d.UpdatePointClouds(depthImage, pointCloudColors);

    // Visualize the skeleton data
    window3d.CleanJointsAndBones();
    uint32_t numBodies = k4abt_frame_get_num_bodies(bodyFrame);
    for (uint32_t i = 0; i < numBodies; i++)
    {
        k4abt_body_t body;
        VERIFY(k4abt_frame_get_body_skeleton(bodyFrame, i, &body.skeleton), "Get skeleton from body frame failed!");
        body.id = k4abt_frame_get_body_id(bodyFrame, i);

        // Assign the correct color based on the body id
        Color color = g_bodyColors[body.id % g_bodyColors.size()];
        color.a = 0.4f;
        Color lowConfidenceColor = color;
        lowConfidenceColor.a = 0.1f;

        //Set up bool and vector to determine arm angle
        bool sel = false;
        bool ewl = false;
        float shoulder_elbow_l[3];
        float elbow_wrist_l[3];

        switch (feature)
        {
            // Quit
        case track_arm:

            // Visualize joints
            for (int joint = 4; joint < 18; joint++)
            {
                if (body.skeleton.joints[joint].confidence_level >= K4ABT_JOINT_CONFIDENCE_LOW)
                {
                    const k4a_float3_t& jointPosition = body.skeleton.joints[joint].position;
                    const k4a_quaternion_t& jointOrientation = body.skeleton.joints[joint].orientation;

                    window3d.AddJoint(
                        jointPosition,
                        jointOrientation,
                        body.skeleton.joints[joint].confidence_level >= K4ABT_JOINT_CONFIDENCE_MEDIUM ? color : lowConfidenceColor);
                }
            }

            // Visualize bones
            for (size_t boneIdx = 6; boneIdx < 12; boneIdx++)
            {
                k4abt_joint_id_t joint1 = g_boneList[boneIdx].first;
                k4abt_joint_id_t joint2 = g_boneList[boneIdx].second;

                if (body.skeleton.joints[joint1].confidence_level >= K4ABT_JOINT_CONFIDENCE_LOW &&
                    body.skeleton.joints[joint2].confidence_level >= K4ABT_JOINT_CONFIDENCE_LOW)
                {
                    bool confidentBone = body.skeleton.joints[joint1].confidence_level >= K4ABT_JOINT_CONFIDENCE_MEDIUM &&
                        body.skeleton.joints[joint2].confidence_level >= K4ABT_JOINT_CONFIDENCE_MEDIUM;
                    const k4a_float3_t& joint1Position = body.skeleton.joints[joint1].position;
                    const k4a_float3_t& joint2Position = body.skeleton.joints[joint2].position;

                    window3d.AddBone(joint1Position, joint2Position, confidentBone ? color : lowConfidenceColor);

                    if (boneIdx == 7) {
                        shoulder_elbow_l[0] = joint2Position.v[0] - joint1Position.v[0];
                        shoulder_elbow_l[1] = joint2Position.v[1] - joint1Position.v[1];
                        shoulder_elbow_l[2] = joint2Position.v[2] - joint1Position.v[2];
                        sel = true;
                    }
                    if (boneIdx == 8) {
                        elbow_wrist_l[0] = joint2Position.v[0] - joint1Position.v[0];
                        elbow_wrist_l[1] = joint2Position.v[1] - joint1Position.v[1];
                        elbow_wrist_l[2] = joint2Position.v[2] - joint1Position.v[2];
                        ewl = true;
                    }
                }
            }

            // Visualize bones
            for (size_t boneIdx = 19; boneIdx < 25; boneIdx++)
            {
                k4abt_joint_id_t joint1 = g_boneList[boneIdx].first;
                k4abt_joint_id_t joint2 = g_boneList[boneIdx].second;

                if (body.skeleton.joints[joint1].confidence_level >= K4ABT_JOINT_CONFIDENCE_LOW &&
                    body.skeleton.joints[joint2].confidence_level >= K4ABT_JOINT_CONFIDENCE_LOW)
                {
                    bool confidentBone = body.skeleton.joints[joint1].confidence_level >= K4ABT_JOINT_CONFIDENCE_MEDIUM &&
                        body.skeleton.joints[joint2].confidence_level >= K4ABT_JOINT_CONFIDENCE_MEDIUM;
                    const k4a_float3_t& joint1Position = body.skeleton.joints[joint1].position;
                    const k4a_float3_t& joint2Position = body.skeleton.joints[joint2].position;

                    window3d.AddBone(joint1Position, joint2Position, confidentBone ? color : lowConfidenceColor);
                }
            }

            if (sel && ewl) {
                float angle = std::acos(dot(shoulder_elbow_l, elbow_wrist_l) / (mag(shoulder_elbow_l) * mag(elbow_wrist_l)));
                float angle_deg = 180.0 - angle * 180.0 / 3.14159265359;

                printf("\nLeft Arm Angle:  %f (degrees)\n", angle_deg);
            }

            sel = ewl = false; //Better luck next time
            break;

        case track_hand:
            // Visualize joints
            for (int joint = 7; joint < 11; joint++)
            {
                if (body.skeleton.joints[joint].confidence_level >= K4ABT_JOINT_CONFIDENCE_LOW)
                {
                    const k4a_float3_t& jointPosition = body.skeleton.joints[joint].position;
                    const k4a_quaternion_t& jointOrientation = body.skeleton.joints[joint].orientation;

                    window3d.AddJoint(
                        jointPosition,
                        jointOrientation,
                        body.skeleton.joints[joint].confidence_level >= K4ABT_JOINT_CONFIDENCE_MEDIUM ? color : lowConfidenceColor);
                }
            }

            // Visualize joints
            for (int joint = 14; joint < 18; joint++)
            {
                if (body.skeleton.joints[joint].confidence_level >= K4ABT_JOINT_CONFIDENCE_LOW)
                {
                    const k4a_float3_t& jointPosition = body.skeleton.joints[joint].position;
                    const k4a_quaternion_t& jointOrientation = body.skeleton.joints[joint].orientation;

                    window3d.AddJoint(
                        jointPosition,
                        jointOrientation,
                        body.skeleton.joints[joint].confidence_level >= K4ABT_JOINT_CONFIDENCE_MEDIUM ? color : lowConfidenceColor);
                }
            }

            // Visualize bones
            for (size_t boneIdx = 9; boneIdx < 12; boneIdx++)
            {
                k4abt_joint_id_t joint1 = g_boneList[boneIdx].first;
                k4abt_joint_id_t joint2 = g_boneList[boneIdx].second;

                if (body.skeleton.joints[joint1].confidence_level >= K4ABT_JOINT_CONFIDENCE_LOW &&
                    body.skeleton.joints[joint2].confidence_level >= K4ABT_JOINT_CONFIDENCE_LOW)
                {
                    bool confidentBone = body.skeleton.joints[joint1].confidence_level >= K4ABT_JOINT_CONFIDENCE_MEDIUM &&
                        body.skeleton.joints[joint2].confidence_level >= K4ABT_JOINT_CONFIDENCE_MEDIUM;
                    const k4a_float3_t& joint1Position = body.skeleton.joints[joint1].position;
                    const k4a_float3_t& joint2Position = body.skeleton.joints[joint2].position;

                    window3d.AddBone(joint1Position, joint2Position, confidentBone ? color : lowConfidenceColor);
                }
            }

            // Visualize bones
            for (size_t boneIdx = 22; boneIdx < 25; boneIdx++)
            {
                k4abt_joint_id_t joint1 = g_boneList[boneIdx].first;
                k4abt_joint_id_t joint2 = g_boneList[boneIdx].second;

                if (body.skeleton.joints[joint1].confidence_level >= K4ABT_JOINT_CONFIDENCE_LOW &&
                    body.skeleton.joints[joint2].confidence_level >= K4ABT_JOINT_CONFIDENCE_LOW)
                {
                    bool confidentBone = body.skeleton.joints[joint1].confidence_level >= K4ABT_JOINT_CONFIDENCE_MEDIUM &&
                        body.skeleton.joints[joint2].confidence_level >= K4ABT_JOINT_CONFIDENCE_MEDIUM;
                    const k4a_float3_t& joint1Position = body.skeleton.joints[joint1].position;
                    const k4a_float3_t& joint2Position = body.skeleton.joints[joint2].position;

                    window3d.AddBone(joint1Position, joint2Position, confidentBone ? color : lowConfidenceColor);
                }
            }
            break;
        case track_face:
            // Visualize joints
            for (int joint = 26; joint < 32; joint++)
            {
                if (body.skeleton.joints[joint].confidence_level >= K4ABT_JOINT_CONFIDENCE_LOW)
                {
                    const k4a_float3_t& jointPosition = body.skeleton.joints[joint].position;
                    const k4a_quaternion_t& jointOrientation = body.skeleton.joints[joint].orientation;

                    window3d.AddJoint(
                        jointPosition,
                        jointOrientation,
                        body.skeleton.joints[joint].confidence_level >= K4ABT_JOINT_CONFIDENCE_MEDIUM ? color : lowConfidenceColor);
                }
            }

            // Visualize bones
            for (size_t boneIdx = 3; boneIdx < 5; boneIdx++)
            {
                k4abt_joint_id_t joint1 = g_boneList[boneIdx].first;
                k4abt_joint_id_t joint2 = g_boneList[boneIdx].second;

                if (body.skeleton.joints[joint1].confidence_level >= K4ABT_JOINT_CONFIDENCE_LOW &&
                    body.skeleton.joints[joint2].confidence_level >= K4ABT_JOINT_CONFIDENCE_LOW)
                {
                    bool confidentBone = body.skeleton.joints[joint1].confidence_level >= K4ABT_JOINT_CONFIDENCE_MEDIUM &&
                        body.skeleton.joints[joint2].confidence_level >= K4ABT_JOINT_CONFIDENCE_MEDIUM;
                    const k4a_float3_t& joint1Position = body.skeleton.joints[joint1].position;
                    const k4a_float3_t& joint2Position = body.skeleton.joints[joint2].position;

                    window3d.AddBone(joint1Position, joint2Position, confidentBone ? color : lowConfidenceColor);
                }
            }

            // Visualize bones
            for (size_t boneIdx = 16; boneIdx < 18; boneIdx++)
            {
                k4abt_joint_id_t joint1 = g_boneList[boneIdx].first;
                k4abt_joint_id_t joint2 = g_boneList[boneIdx].second;

                if (body.skeleton.joints[joint1].confidence_level >= K4ABT_JOINT_CONFIDENCE_LOW &&
                    body.skeleton.joints[joint2].confidence_level >= K4ABT_JOINT_CONFIDENCE_LOW)
                {
                    bool confidentBone = body.skeleton.joints[joint1].confidence_level >= K4ABT_JOINT_CONFIDENCE_MEDIUM &&
                        body.skeleton.joints[joint2].confidence_level >= K4ABT_JOINT_CONFIDENCE_MEDIUM;
                    const k4a_float3_t& joint1Position = body.skeleton.joints[joint1].position;
                    const k4a_float3_t& joint2Position = body.skeleton.joints[joint2].position;

                    window3d.AddBone(joint1Position, joint2Position, confidentBone ? color : lowConfidenceColor);
                }
            }

            // Visualize bones
            for (size_t boneIdx = 29; boneIdx < 31; boneIdx++)
            {
                k4abt_joint_id_t joint1 = g_boneList[boneIdx].first;
                k4abt_joint_id_t joint2 = g_boneList[boneIdx].second;

                if (body.skeleton.joints[joint1].confidence_level >= K4ABT_JOINT_CONFIDENCE_LOW &&
                    body.skeleton.joints[joint2].confidence_level >= K4ABT_JOINT_CONFIDENCE_LOW)
                {
                    bool confidentBone = body.skeleton.joints[joint1].confidence_level >= K4ABT_JOINT_CONFIDENCE_MEDIUM &&
                        body.skeleton.joints[joint2].confidence_level >= K4ABT_JOINT_CONFIDENCE_MEDIUM;
                    const k4a_float3_t& joint1Position = body.skeleton.joints[joint1].position;
                    const k4a_float3_t& joint2Position = body.skeleton.joints[joint2].position;

                    window3d.AddBone(joint1Position, joint2Position, confidentBone ? color : lowConfidenceColor);
                }
            }
            break;
        case track_body:
            // Visualize joints
            for (int joint = 0; joint < static_cast<int>(K4ABT_JOINT_COUNT); joint++)
            {
                if (body.skeleton.joints[joint].confidence_level >= K4ABT_JOINT_CONFIDENCE_LOW)
                {
                    const k4a_float3_t& jointPosition = body.skeleton.joints[joint].position;
                    const k4a_quaternion_t& jointOrientation = body.skeleton.joints[joint].orientation;

                    window3d.AddJoint(
                        jointPosition,
                        jointOrientation,
                        body.skeleton.joints[joint].confidence_level >= K4ABT_JOINT_CONFIDENCE_MEDIUM ? color : lowConfidenceColor);
                }
            }

            // Visualize bones
            for (size_t boneIdx = 0; boneIdx < g_boneList.size(); boneIdx++)
            {
                k4abt_joint_id_t joint1 = g_boneList[boneIdx].first;
                k4abt_joint_id_t joint2 = g_boneList[boneIdx].second;

                if (body.skeleton.joints[joint1].confidence_level >= K4ABT_JOINT_CONFIDENCE_LOW &&
                    body.skeleton.joints[joint2].confidence_level >= K4ABT_JOINT_CONFIDENCE_LOW)
                {
                    bool confidentBone = body.skeleton.joints[joint1].confidence_level >= K4ABT_JOINT_CONFIDENCE_MEDIUM &&
                        body.skeleton.joints[joint2].confidence_level >= K4ABT_JOINT_CONFIDENCE_MEDIUM;
                    const k4a_float3_t& joint1Position = body.skeleton.joints[joint1].position;
                    const k4a_float3_t& joint2Position = body.skeleton.joints[joint2].position;

                    window3d.AddBone(joint1Position, joint2Position, confidentBone ? color : lowConfidenceColor);
                }
            }
            break;
        }

    }

    k4a_capture_release(originalCapture);
    k4a_image_release(depthImage);

}

void PlayFile(InputSettings inputSettings) {
    // Initialize the 3d window controller
    Window3dWrapper window3d;

    //create the tracker and playback handle
    k4a_calibration_t sensor_calibration;
    k4abt_tracker_t tracker = NULL;
    k4a_playback_t playback_handle = NULL;

    const char* file = inputSettings.FileName.c_str();
    if (k4a_playback_open(file, &playback_handle) != K4A_RESULT_SUCCEEDED)
    {
        printf("Failed to open recording: %s\n", file);
        return;
    }


    if (k4a_playback_get_calibration(playback_handle, &sensor_calibration) != K4A_RESULT_SUCCEEDED)
    {
        printf("Failed to get calibration\n");
        return;
    }
    

    k4a_capture_t capture = NULL;
    k4a_stream_result_t result = K4A_STREAM_RESULT_SUCCEEDED;

    k4abt_tracker_configuration_t tracker_config = { K4ABT_SENSOR_ORIENTATION_DEFAULT };

    tracker_config.processing_mode = inputSettings.CpuOnlyMode ? K4ABT_TRACKER_PROCESSING_MODE_CPU : K4ABT_TRACKER_PROCESSING_MODE_GPU;

    VERIFY(k4abt_tracker_create(&sensor_calibration, tracker_config, &tracker), "Body tracker initialization failed!");

    k4abt_tracker_set_temporal_smoothing(tracker, 1);

    int depthWidth = sensor_calibration.depth_camera_calibration.resolution_width;
    int depthHeight = sensor_calibration.depth_camera_calibration.resolution_height;

    window3d.Create("3D Visualization", sensor_calibration);
    window3d.SetCloseCallback(CloseCallback);
    window3d.SetKeyCallback(ProcessKey);

    while (result == K4A_STREAM_RESULT_SUCCEEDED)
    {
        result = k4a_playback_get_next_capture(playback_handle, &capture);
        // check to make sure we have a depth image
        // if we are not at the end of the file
        if (result != K4A_STREAM_RESULT_EOF) {
            k4a_image_t depth_image = k4a_capture_get_depth_image(capture);
            if (depth_image == NULL) {
                //If no depth image, print a warning and skip to next frame
                printf("Warning: No depth image, skipping frame\n");
                k4a_capture_release(capture);
                continue;
            }
            // Release the Depth image
            k4a_image_release(depth_image);
        }
        if (result == K4A_STREAM_RESULT_SUCCEEDED)
        {
            
            //enque capture and pop results - synchronous
            k4a_wait_result_t queue_capture_result = k4abt_tracker_enqueue_capture(tracker, capture, K4A_WAIT_INFINITE);

            // Release the sensor capture once it is no longer needed.
            k4a_capture_release(capture);

            k4abt_frame_t bodyFrame = NULL;
            k4a_wait_result_t pop_frame_result = k4abt_tracker_pop_result(tracker, &bodyFrame, K4A_WAIT_INFINITE);
            if (pop_frame_result == K4A_WAIT_RESULT_SUCCEEDED)
            {
                size_t num_bodies = k4abt_frame_get_num_bodies(bodyFrame);
                printf("%zu bodies are detected\n", num_bodies);
                /************* Successfully get a body tracking result, process the result here ***************/
                VisualizeResult(bodyFrame, window3d, depthWidth, depthHeight); 
                //Release the bodyFrame
                k4abt_frame_release(bodyFrame);
            }
            else
            {
                printf("Pop body frame result failed!\n");
                break;
            }
           
        }

        window3d.SetLayout3d(s_layoutMode);
        window3d.SetJointFrameVisualization(s_visualizeJointFrame);
        window3d.Render();

        if (result == K4A_STREAM_RESULT_EOF)
        {
            // End of file reached
            break;
        }
    }
    k4abt_tracker_shutdown(tracker);
    k4abt_tracker_destroy(tracker);
    window3d.Delete();
    printf("Finished body tracking processing!\n");
    k4a_playback_close(playback_handle);

}

void PlayFromDevice(InputSettings inputSettings) {
    k4a_device_t device = nullptr;
    VERIFY(k4a_device_open(0, &device), "Open K4A Device failed!");

    // Start camera. Make sure depth camera is enabled.
    k4a_device_configuration_t deviceConfig = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    deviceConfig.depth_mode = inputSettings.DepthCameraMode;
    deviceConfig.color_resolution = K4A_COLOR_RESOLUTION_OFF;
    VERIFY(k4a_device_start_cameras(device, &deviceConfig), "Start K4A cameras failed!");

    // Get calibration information
    k4a_calibration_t sensorCalibration;
    VERIFY(k4a_device_get_calibration(device, deviceConfig.depth_mode, deviceConfig.color_resolution, &sensorCalibration),
        "Get depth camera calibration failed!");
    int depthWidth = sensorCalibration.depth_camera_calibration.resolution_width;
    int depthHeight = sensorCalibration.depth_camera_calibration.resolution_height;

    // Create Body Tracker
    k4abt_tracker_t tracker = nullptr;
    k4abt_tracker_configuration_t tracker_config = K4ABT_TRACKER_CONFIG_DEFAULT;
    tracker_config.processing_mode = inputSettings.CpuOnlyMode ? K4ABT_TRACKER_PROCESSING_MODE_CPU : K4ABT_TRACKER_PROCESSING_MODE_GPU;
    VERIFY(k4abt_tracker_create(&sensorCalibration, tracker_config, &tracker), "Body tracker initialization failed!");
    // Initialize the 3d window controller
    Window3dWrapper window3d;
    window3d.Create("3D Visualization", sensorCalibration);
    window3d.SetCloseCallback(CloseCallback);
    window3d.SetKeyCallback(ProcessKey);

    while (s_isRunning)
    {
        k4a_capture_t sensorCapture = nullptr;
        k4a_wait_result_t getCaptureResult = k4a_device_get_capture(device, &sensorCapture, 0); // timeout_in_ms is set to 0

        if (getCaptureResult == K4A_WAIT_RESULT_SUCCEEDED)
        {
            // timeout_in_ms is set to 0. Return immediately no matter whether the sensorCapture is successfully added
            // to the queue or not.
            k4a_wait_result_t queueCaptureResult = k4abt_tracker_enqueue_capture(tracker, sensorCapture, 0);

            // Release the sensor capture once it is no longer needed.
            k4a_capture_release(sensorCapture);

            if (queueCaptureResult == K4A_WAIT_RESULT_FAILED)
            {
                std::cout << "Error! Add capture to tracker process queue failed!" << std::endl;
                break;
            }
        }
        else if (getCaptureResult != K4A_WAIT_RESULT_TIMEOUT)
        {
            std::cout << "Get depth capture returned error: " << getCaptureResult << std::endl;
            break;
        }

        // Pop Result from Body Tracker
        k4abt_frame_t bodyFrame = nullptr;
        k4a_wait_result_t popFrameResult = k4abt_tracker_pop_result(tracker, &bodyFrame, 0); // timeout_in_ms is set to 0
        if (popFrameResult == K4A_WAIT_RESULT_SUCCEEDED)
        {
            /************* Successfully get a body tracking result, process the result here ***************/
            VisualizeResult(bodyFrame, window3d, depthWidth, depthHeight);
            //Release the bodyFrame
            k4abt_frame_release(bodyFrame);
        }
       
        window3d.SetLayout3d(s_layoutMode);
        window3d.SetJointFrameVisualization(s_visualizeJointFrame);
        window3d.Render();
    }

    std::cout << "Finished body tracking processing!" << std::endl;

    window3d.Delete();
    k4abt_tracker_shutdown(tracker);
    k4abt_tracker_destroy(tracker);

    k4a_device_stop_cameras(device);
    k4a_device_close(device);


}

string recognizeSpeech() {
    // Creates an instance of a speech config with specified subscription key and service region.
    // Replace with your own subscription key and service region (e.g., "westus").
    auto config = SpeechConfig::FromSubscription("116da67120414932baf397db6eb136c5", "eastus");

    // Creates a speech recognizer
    auto recognizer = SpeechRecognizer::FromConfig(config);
    cout << "Say something...\n";

    // Starts speech recognition, and returns after a single utterance is recognized. The end of a
    // single utterance is determined by listening for silence at the end or until a maximum of 15
    // seconds of audio is processed.  The task returns the recognition text as result. 
    // Note: Since RecognizeOnceAsync() returns only a single utterance, it is suitable only for single
    // shot recognition like command or query. 
    // For long-running multi-utterance recognition, use StartContinuousRecognitionAsync() instead.
    auto result = recognizer->RecognizeOnceAsync().get();

    // Checks result.
    if (result->Reason == ResultReason::RecognizedSpeech) {
        cout << "Processing..." << std::endl;
    }
    else if (result->Reason == ResultReason::NoMatch) {
        cout << "NOMATCH: Speech could not be recognized." << std::endl;
    }
    else if (result->Reason == ResultReason::Canceled) {
        auto cancellation = CancellationDetails::FromResult(result);
        cout << "CANCELED: Reason=" << (int)cancellation->Reason << std::endl;

        if (cancellation->Reason == CancellationReason::Error) {
            cout << "CANCELED: ErrorCode= " << (int)cancellation->ErrorCode << std::endl;
            cout << "CANCELED: ErrorDetails=" << cancellation->ErrorDetails << std::endl;
            cout << "CANCELED: Did you update the subscription info?" << std::endl;
        }
    }
    return result->Text;
}

void synthesizeSpeech(string str)
{
    auto config = SpeechConfig::FromSubscription("116da67120414932baf397db6eb136c5", "eastus");
    auto synthesizer = SpeechSynthesizer::FromConfig(config);
    auto result = synthesizer->SpeakTextAsync(str).get();
}

int main(int argc, char** argv)
{
    InputSettings inputSettings;
    string str = "sleep";
    bool wake = false;
    do
    { 
        string str_check = "Hello Kitty";
        string str = recognizeSpeech();
   
        //cout << "Double checking: " << str << std::endl;

        if (str.find(str_check) != std::string::npos) {
            std::cout << "Processed." << '\n';
            wake = true;
        }

    } while (wake != true);

    synthesizeSpeech("Hello father. I am waking up from my slumber.");

    if (ParseInputSettingsFromArg(argc, argv, inputSettings)) {
        // Either play the offline file or play from the device
        if (inputSettings.Offline == true) {     
            PlayFile(inputSettings);
        }
        else {
            PlayFromDevice(inputSettings);
        }
    }
    else {
        // Print app usage if user entered incorrect arguments.
        PrintUsage();
        return -1;
    }

    return 0;
}
