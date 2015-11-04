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

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import com.project.tango.R;

import android.app.Activity;
import android.content.ContextWrapper;
import android.content.res.AssetManager;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.View;
import android.widget.Toast;
import android.widget.ToggleButton;
import android.util.Log;

// Main activity
public class TangoARActivity extends Activity
    implements View.OnClickListener {
	
  public enum TextureMethod {
    YUV,
    TEXTURE_ID
  }

  private static final int RET_SUCCESS 			= 0;
  private static final int RET_FAILED 			= -1;
  private static final String TAG 				= "TangoAR-java";
  private static final String mModelFilename 	= "model.bin";
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

    // Copy model binary file from asset to sd card so that 
    // the native side can read it.
    ContextWrapper pContextW  = new ContextWrapper(this);
	String path = pContextW.getFilesDir().getAbsolutePath();
	File MDLData = new File(path + mModelFilename);
	if(MDLData.exists()){
		Log.e(TAG, "Model file is not in the assets!\n");
	}
	else{
		copyAssets(path);
	}
	
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
    if(loadBinaryModel() != RET_SUCCESS){
    		Log.i(TAG, "Unable to load model file \n");
    		Toast.makeText(getBaseContext(),
    				"Unable to load model!", Toast.LENGTH_SHORT).show();
    }else{
    	Toast.makeText(getBaseContext(),
    			"Model loaded ...", Toast.LENGTH_SHORT).show();
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
		return TangoJNINative.loadTargetModel(path + "/" + mModelFilename);
  }
  
  private void copyAssets(String outputPath) {
	  AssetManager assetManager = getAssets();
	  String[] files = null;
	  try {
		  files = assetManager.list("");
	  }
	  catch (IOException e) {
		  Log.e("TAG", "Failed to get asset file list.", e);
	  }

	  for(String filename : files) {
		  if( filename.equals(mModelFilename) )
		  {
			  InputStream inStream = null;
			  OutputStream outStream = null;

			  try {
				  inStream = assetManager.open(filename);
				  String out= outputPath +"/" +filename ; 

				  File outFile = new File(out);
				  outStream = new FileOutputStream(outFile);

				  copyFile(inStream, outStream);
				  inStream.close();
				  inStream = null;
				  outStream.flush();
				  outStream.close();
				  outStream = null;
			  }
			  catch(IOException e) {
				  Log.e(TAG, "Failed to copy asset file: " + filename, e);
			  }
		  }
	  }
  }
  private void copyFile(InputStream in, OutputStream out) throws IOException {
	  byte[] buffer = new byte[1024];
	  int read;
	  while((read = in.read(buffer)) != -1){
		  out.write(buffer, 0, read);
	  }
  }
}