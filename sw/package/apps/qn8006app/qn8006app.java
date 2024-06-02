/*this is a app demo*/
package com.android.qn8006app;
 
import android.os.Bundle;
import android.app.Activity;
import android.view.Menu;
 
import android.os.IQn8006Service;
import android.os.ServiceManager;
import android.os.RemoteException;
import android.util.Log;
 
public class MainActivity extends Activity  {
	 
	    private static final String TAG = "Qn8006App";
	    private IQn8006Service qn8006Service = null;
	    private int loop=128;
	 
	    private int i;
	    private int nCH;
	 
	    private int tmprdsinfo=0;
	    private int reg_val;
	    private int [] buffer =  {0,1,2,3,4,5,6,7};
	    private int retval;
	 
	    private void myDelay(int ms) {
		        try {
			            Thread.currentThread();
			            Thread.sleep(ms);
			        
		}
		        catch(InterruptedException e) {
			            e.printStackTrace();
			        
		}
		    
	}
	  
	 
	    @Override
	    protected void onCreate(Bundle savedInstanceState)  {
		        super.onCreate(savedInstanceState);
		        setContentView(R.layout.activity_main);
		 
		            Log.d("Qn8006App", "  onCreate func");
		            qn8006Service = IQn8006Service.Stub.asInterface(ServiceManager.getService("qn8006"));
		 
		 
		            if(true) {
			            try  {
				                    qn8006Service.apiQn8006powerEnable(1);
				 
				                    qn8006Service.apiQn8006csPin(1);
				                    qn8006Service.Init();
				                    qn8006Service.setSystemMode(0x8000);
				 //0x4000
				                    qn8006Service.setFrequency(9150);
				                    qn8006Service.apiQn8006RDSEnable(1);
				                    qn8006Service.apiQn8006RDSLoadData(buffer, 8, 1);
				                    retval = qn8006Service.apiQn8006RDSCheckBufferReady();
				                    Log.d(TAG, "----d.j----detect chkbufrdy= [" + retval + "] ----");
				 
				            
			}
			            catch(Exception ex) {
				            
			}
			            
		}
		 
		 
//            if(false) { /* problem */
//          try {
//                    qn8006Service.apiQn8006powerEnable(1);
//
//                    qn8006Service.apiQn8006csPin(1);
//                    qn8006Service.Init();
//          		  qn8006Service.setSystemMode(0x8000); //0x4000
//                    qn8006Service.setFrequency(9150);
//                    qn8006Service.apiQn8006RDSEnable(1);
//                    
//                    for(loop=128; loop>0; loop--){
//                        myDelay(5);
//                        reg_val = qn8006Service.apiQn8006RDSDetectSignal();
//                        Log.d(TAG, "----d.j--111--detect signal [" + reg_val + "] ");
//                        reg_val = reg_val >> 7;
//                        Log.d(TAG, "----d.j--222--detect signal [" + reg_val + "] ");
//                        //if( reg_val^tmprdsinfo )
//                        {
//                            qn8006Service.apiQn8006RDSLoadData(buffer, 8, 0);
//                            Log.d(TAG, "----d.j----recv rds data: [ " + buffer[0] + " " + buffer[1] + " " + buffer[2] + " " + buffer[3] + " " + buffer[4] + " " + buffer[5] + " " + buffer[6] + " " + buffer[7] + " " + " ] ");
//                        }
//
//                   }
//
//          }
//          catch(Exception ex){
//          }
//            }
		 
		            if(false) {
			            try  {
				                    qn8006Service.apiQn8006powerEnable(1);
				 
				                    qn8006Service.apiQn8006csPin(1);
				                    qn8006Service.Init();
				                    qn8006Service.setSystemMode(16384);
				 //0x4000
				                    qn8006Service.setFrequency(8800);
				                    qn8006Service.setPower(0);
				                    qn8006Service.setAudioMode(0);
				 
				                    nCH = qn8006Service.apiQn8006rxSeekCHAll(7600, 10800, 1, 6, 1);
				                    Log.d(TAG, "----d.j----have [" + nCH + "] channels been found");
				 
				            
			}
			            catch(Exception ex) {
				            
			}
			            
		}
		 
		            if(false) {
			            try  {
				                    qn8006Service.apiQn8006powerEnable(1);
				                    qn8006Service.apiQn8006csPin(1);
				                    qn8006Service.Init();
				                    qn8006Service.setSystemMode(16384);
				 //0x4000
				                    qn8006Service.setFrequency(8800);
				                    qn8006Service.setPower(0);
				                    qn8006Service.setAudioMode(0);
				            
			}
			            catch(Exception ex) {
				            
			}
			            
		}
		    
	}
	 
	    @Override
	    public boolean onCreateOptionsMenu(Menu menu)  {
		        // Inflate the menu; this adds items to the action bar if it is present.
		        getMenuInflater().inflate(R.menu.main, menu);
		        return true;
		    
	}
	 
}
