/*
 * Copyright 2014 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef TANGO_VIDEO_OVERLAY_VIDEO_OVERLAY_APP_H_
#define TANGO_VIDEO_OVERLAY_VIDEO_OVERLAY_APP_H_

#include <atomic>
#include <jni.h>
#include <memory>
#include <mutex>

#include <tango_client_api.h>  // NOLINT
#include <tango-gl/util.h>
#include <tango-gl/video_overlay.h>
#include <tango-gl/cube.h>
#include <tango-gl/goal_marker.h>
#include <tango-gl/camera.h>
#include <tango-gl/conversions.h>
#include "param.h"
#include "yuv_drawable.h"
#include "frame_processor.h"

namespace tango_video_overlay {

// VideoOverlayApp handles the application lifecycle and resources.
class VideoOverlayApp {
 public:
  // Constructor and deconstructor.
  VideoOverlayApp();
  ~VideoOverlayApp();

  enum TextureMethod {
    kYUV,
    kTextureId
  };

  // YUV data callback.
  void OnFrameAvailable(const TangoImageBuffer* buffer);

  // Initialize Tango Service, this function starts the communication
  // between the application and Tango Service.
  // The activity object is used for checking if the API version is outdated.
  int TangoInitialize(JNIEnv* env, jobject caller_activity);

  // Setup the configuration file for the Tango Service. We'll also se whether
  // we'd like auto-recover enabled.
  int TangoSetupConfig();

  // Connect to the Tango Service.
  // This function will start the Tango Service pipeline, in this case, it will
  // start the video overlay update.
  int TangoConnect();

  // Disconnect from Tango Service, release all the resources that the app is
  // holding from Tango Service.
  void TangoDisconnect();

  // Allocate OpenGL resources for rendering, mainly initializing the Scene.
  void InitializeGLContent();

  // Setup the view port width and height.
  void SetViewPort(int width, int height);

  // Main render loop.
  void Render();

  // Release all OpenGL resources that allocate from the program.
  void FreeGLContent();

  // Set texture method.
  void SetTextureMethod(int method) {
    current_texture_method_ = static_cast<TextureMethod>(method);
  }

  // Set projection matrix of the AR view (first person view)
  // @param: projection_matrix, the projection matrix.
  void SetARCameraProjectionMatrix(const glm::mat4& projection_matrix) {
    ar_camera_projection_matrix_ = projection_matrix;
  }
  glm::mat4 GetARCameraProjectionMatrix() {
      return ar_camera_projection_matrix_;
    }
  // Load visual features from the binary file
  int LoadTargetModel(JNIEnv* env, jstring path);

 private:
  // The projection matrix for the first person AR camera.
  glm::mat4 ar_camera_projection_matrix_;

  // Tango configration file, this object is for configuring Tango Service setup
  // before connect to service. For example, we set the flag
  // config_enable_auto_recovery based user's input and then start Tango.
  TangoConfig tango_config_;

  // video_overlay_ render the camera video feedback onto the screen.
  tango_gl::VideoOverlay* video_overlay_drawable_;
  tango_gl::Cube* _cube;
  YUVDrawable* yuv_drawable_;
  tango_gl::GoalMarker* _marker;
  TangoCameraIntrinsics color_camera_intrinsics_;

  TextureMethod current_texture_method_;

  std::vector<uint8_t> yuv_buffer_;
  std::vector<uint8_t> yuv_temp_buffer_;
  std::vector<GLubyte> rgb_buffer_;

  std::atomic<bool> is_yuv_texture_available_;
  std::atomic<bool> swap_buffer_signal_;
  std::mutex yuv_buffer_mutex_;

  size_t yuv_width_;
  size_t yuv_height_;
  size_t yuv_size_;
  size_t uv_buffer_offset_;

  void AllocateTexture(GLuint texture_id, int width, int height);
  void RenderYUV();
  void RenderTextureId();
};
}  // namespace tango_video_overlay

#endif  // TANGO_VIDEO_OVERLAY_VIDEO_OVERLAY_APP_H_
