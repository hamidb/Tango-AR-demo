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

package com.project.tango;

import com.project.tango.R;

import android.app.Activity;
import android.content.ContextWrapper;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.View;
import android.widget.Toast;
import android.widget.ToggleButton;
import android.util.Log;
import android.util.Log;
// Main activity
public class TangoARActivity extends Activity
    implements View.OnClickListener {
	
  public enum TextureMethod {
    YUV,
    TEXTURE_ID
  }

  private static final String TAG = "TangoAR-java";
  private GLSurfaceView glView;
  private ToggleButton mYUVRenderSwitcher;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    setContentView(R.layout.activity_video_overlay);
    glView = (GLSurfaceView) findViewById(R.id.surfaceview);
    glView.setRenderer(new Renderer());

    mYUVRenderSwitcher = (ToggleButton) findViewById(R.id.yuv_switcher);
    mYUVRenderSwitcher.setOnClickListener(this);

    // Initialize Tango Service, this function starts the communication
    // between the application and Tango Service.
    // The activity object is used for checking if the API version is outdated.
    TangoJNINative.initialize(this);
  }
  
  @Override
  protected void onResume() {
    super.onResume();
    glView.onResume();
    // Setup the configuration for the TangoService.
    TangoJNINative.setupConfig();

    // Connect to Tango Service.
    // This function will start the Tango Service pipeline, in this case,
    // it will start Motion Tracking.
    TangoJNINative.connect();    
	
    // Load binary model containing visual description of the target.
    if(loadBinaryModel() != 1){
    		Log.i(TAG, "Unable to load model file \n");
    		Toast.makeText(getBaseContext(),
    				"Unable to load model!", Toast.LENGTH_SHORT).show();
    }

    EnableYUVTexture(mYUVRenderSwitcher.isChecked());
  }

  @Override
  protected void onPause() {
    super.onPause();
    glView.onPause();
    // Disconnect from Tango Service, release all the resources that the app is
    // holding from Tango Service.
    TangoJNINative.disconnect();
    TangoJNINative.freeGLContent();
  }

  @Override
  public void onClick(View v) {
    switch (v.getId()) {
    case R.id.yuv_switcher:
      EnableYUVTexture(mYUVRenderSwitcher.isChecked());
      break;
    }
  }

  @Override
  protected void onDestroy() {	
    super.onDestroy();
  }

  private void EnableYUVTexture(boolean isEnabled) {
    if (isEnabled) {
        // Turn on YUV
        TangoJNINative.setYUVMethod();
      } else {
        TangoJNINative.setTextureMethod();
      }
  }
  
  private int loadBinaryModel(){
		ContextWrapper cw = new ContextWrapper(getBaseContext());
		String path = cw.getFilesDir().getAbsolutePath();		
		return TangoJNINative.loadTargetModel(path+"/model.bin");
  }
}