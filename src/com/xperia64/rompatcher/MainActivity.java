/*******************************************************************************
 * This file is part of ROM Patcher.
 * 
 * Copyright (c) 2014 xperia64.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Public License v3.0
 * which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/gpl.html
 * 
 * Contributors:
 * 	Paul Kratt - main MultiPatch application for Mac OSX
 * 	byuu - UPS and BPS patchers
 * 	Neil Corlett - IPS patcher
 * 	Daniel Ekstr'm - PPF patcher
 * 	Josh MacDonald - XDelta
 * 	Colin Percival - BSDiff
 * 	xperia64 - port to Android and IPS32 support
 ******************************************************************************/
package com.xperia64.rompatcher;




import com.google.common.io.Files;
import com.xperia64.rompatcher.R;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.util.Locale;

import android.text.InputFilter;
import android.util.Log;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.Toast;
import android.text.Spanned;

public class MainActivity extends Activity {


	public static native int ipsPatchRom(String romPath, String patchPath); 
	public static native int ips32PatchRom(String romPath, String patchPath); 
	public static native int upsPatchRom(String romPath, String patchPath, String outputFile);
	public static native int xdeltaPatchRom(String romPath, String patchPath, String outputFile);
	public static native int bsdiffPatchRom(String romPath, String patchPath, String outputFile);
	public static native int bpsPatchRom(String romPath, String patchPath, String outputFile);
	public static native int ppfPatchRom(String romPath, String patchPath);
	Context staticThis;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		staticThis=MainActivity.this;
		// Load native libraries
		try {
			System.loadLibrary("ipspatcher");  
		}
		catch( UnsatisfiedLinkError e) {
			Log.e("Bad:","Cannot grab ipspatcher!");
		}
		try {
			System.loadLibrary("upspatcher");  
		}
		catch( UnsatisfiedLinkError e) {
			Log.e("Bad:","Cannot grab upspatcher!");
		}
		try {
			System.loadLibrary("xdelta3patcher");  
		}
		catch( UnsatisfiedLinkError e) {
			Log.e("Bad:","Cannot grab xdelta3patcher!");
		}
		try {
			System.loadLibrary("bpspatcher");  
		}
		catch( UnsatisfiedLinkError e) {
			Log.e("Bad:","Cannot grab bpspatcher!");
		}
		try {
			System.loadLibrary("bzip2");  
		}
		catch( UnsatisfiedLinkError e) {
			Log.e("Bad:","Cannot grab bzip2!");
		}
		try {
			System.loadLibrary("bsdiffpatcher");  
		}
		catch( UnsatisfiedLinkError e) {
			Log.e("Bad:","Cannot grab bsdiffpatcher!");
		}
		try {
			System.loadLibrary("ppfpatcher");  
		}
		catch( UnsatisfiedLinkError e) {
			Log.e("Bad:","Cannot grab ppfpatcher!");
		}
		try {
			System.loadLibrary("ips32patcher");  
		}
		catch( UnsatisfiedLinkError e) {
			Log.e("Bad:","Cannot grab ips32patcher!");
		}
		setContentView(R.layout.main);
		final Button romButton = (Button) findViewById(R.id.romButton);
		romButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				Globals.mode=true;
				Intent intent = new Intent( staticThis, FileBrowserActivity.class );
				intent.setFlags( Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP );
				startActivityForResult( intent,1 );


			}
		});
		final Button patchButton = (Button) findViewById(R.id.patchButton);
		patchButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				Globals.mode=false;
				Intent intent = new Intent( staticThis, FileBrowserActivity.class );
				intent.setFlags( Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP );
				startActivityForResult( intent, 1 );
			}
		});
		final ImageButton bkHelp = (ImageButton) findViewById(R.id.backupHelp);
		bkHelp.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				AlertDialog dialog = new AlertDialog.Builder(staticThis).create();
			    dialog.setTitle(getResources().getString(R.string.bkup_rom));
			    dialog.setMessage(getResources().getString(R.string.bkup_rom_desc));
			    dialog.setCancelable(true);
			    dialog.setButton(DialogInterface.BUTTON_POSITIVE, getResources().getString(android.R.string.ok), new DialogInterface.OnClickListener() {
			        public void onClick(DialogInterface dialog, int buttonId) {

			        }
			    });
			    dialog.show();
			}
		});
		final ImageButton altNameHelp = (ImageButton) findViewById(R.id.outfileHelp);
		altNameHelp.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				AlertDialog dialog = new AlertDialog.Builder(staticThis).create();
			    dialog.setTitle(getResources().getString(R.string.rename1));
			    dialog.setMessage(getResources().getString(R.string.rename_desc));
			    dialog.setCancelable(true);
			    dialog.setButton(DialogInterface.BUTTON_POSITIVE, getResources().getString(android.R.string.ok), new DialogInterface.OnClickListener() {
			        public void onClick(DialogInterface dialog, int buttonId) {

			        }
			    });
			    dialog.show();
			}
		});
		final CheckBox c = (CheckBox) findViewById(R.id.backupCheckbox);
		final CheckBox d = (CheckBox) findViewById(R.id.altNameCheckbox);
		final EditText ed = (EditText) findViewById(R.id.txtOutFile);
		InputFilter filter = new InputFilter() { 
	        public CharSequence filter(CharSequence source, int start, int end, 
	Spanned dest, int dstart, int dend) { 
	                for (int i = start; i < end; i++) { 
	                	String IC = "*/*\n*\r*\t*\0*\f*`*?***\\*<*>*|*\"*:*";
	                        if (IC.contains("*"+source.charAt(i)+"*")) { 
	                                return ""; 
	                        } 
	                } 
	                return null; 
	        } 
		};
		ed.setFilters(new InputFilter[]{filter});
		c.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				if(((CheckBox)v).isChecked())
				{
					d.setEnabled(true);
					if(d.isChecked())
					{
						ed.setEnabled(true);
					}
				}else{
					d.setEnabled(false);
					ed.setEnabled(false);
				}
			}});
		d.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				if(((CheckBox)v).isChecked())
				{
					ed.setEnabled(true);
				}else{				
					ed.setEnabled(false);
				}
			}});
		final Button applyButton = (Button) findViewById(R.id.applyPatch);
		applyButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				if(new File(Globals.fileToPatch+".bak").exists()||(new File(Globals.fileToPatch.substring(0,Globals.fileToPatch.lastIndexOf('/')+1)+ed.getText().toString()).exists()&&d.isChecked()&&c.isChecked()))
				{
					AlertDialog dialog = new AlertDialog.Builder(staticThis).create();
				    dialog.setTitle(getResources().getString(R.string.warning));
				    dialog.setMessage(getResources().getString(R.string.warning_desc));
				    dialog.setCancelable(false);
				    dialog.setButton(DialogInterface.BUTTON_POSITIVE, getResources().getString(android.R.string.yes), new DialogInterface.OnClickListener() {
				        public void onClick(DialogInterface dialog, int buttonId) {
				            new File(Globals.fileToPatch+".bak").delete();
				            new File(Globals.fileToPatch.substring(0,Globals.fileToPatch.lastIndexOf('/')+1)+ed.getText().toString()).delete();
				            patch(c.isChecked(),d.isChecked(),ed.getText().toString());
				        }
				    });
				    dialog.setButton(DialogInterface.BUTTON_NEGATIVE, getResources().getString(android.R.string.cancel), new DialogInterface.OnClickListener() {
				        public void onClick(DialogInterface dialog, int buttonId) {
				          Toast t  = Toast.makeText(staticThis, getResources().getString(R.string.nopatch), Toast.LENGTH_SHORT);
				          t.show();
				        }
				    });
				    dialog.show();
				}else{
					patch(c.isChecked(),d.isChecked(),ed.getText().toString());
				}	
			}
		});
	}
	
	public void patch(final boolean c, final boolean d, final String ed)
	{
		final ProgressDialog myPd_ring=ProgressDialog.show(staticThis, getResources().getString(R.string.wait), getResources().getString(R.string.wait_desc), true);
        myPd_ring.setCancelable(false);
			new Thread(new Runnable() {
		        public void run(){
		        	if(new File(Globals.patchToApply).exists()&& new File(Globals.fileToPatch).exists()){
		        		String msg=getResources().getString(R.string.success);
						if(Globals.patchToApply.toLowerCase(Locale.US).endsWith(".ups"))
						{
							int e = upsPatchRom(Globals.fileToPatch, Globals.patchToApply, Globals.fileToPatch+".new");
							if(e!=0)
							{
								msg=parseError(e, Globals.TYPE_UPS);
							}
						}else if(Globals.patchToApply.toLowerCase(Locale.US).endsWith(".xdelta")){
							int e = xdeltaPatchRom(Globals.fileToPatch, Globals.patchToApply, Globals.fileToPatch+".new");
							if(e!=0)
							{
								msg=parseError(e, Globals.TYPE_XDELTA);
							}
						}else if(Globals.patchToApply.toLowerCase(Locale.US).endsWith(".bps")){
							int e = bpsPatchRom(Globals.fileToPatch, Globals.patchToApply, Globals.fileToPatch+".new");
							if(e!=0)
							{
								msg=parseError(e, Globals.TYPE_BPS);
							}
						}else if(Globals.patchToApply.toLowerCase(Locale.US).endsWith(".bsdiff")){
							int e = bsdiffPatchRom(Globals.fileToPatch, Globals.patchToApply, Globals.fileToPatch+".new");
							if(e!=0)
							{
								msg=parseError(e, Globals.TYPE_BSDIFF);
							}
						}else if(Globals.patchToApply.toLowerCase(Locale.US).endsWith(".ppf")){
							File f = new File(Globals.fileToPatch);
							File f2 = new File(Globals.fileToPatch+".bak");
							try {
								Files.copy(f,f2);
							} catch (IOException e) {
								e.printStackTrace();
							}
							int e = ppfPatchRom(Globals.fileToPatch, Globals.patchToApply);
							if(e!=0)
							{
								msg=parseError(e, Globals.TYPE_PPF);
							}
						}else{
							RandomAccessFile f= null;
							try {
								f = new RandomAccessFile(Globals.patchToApply, "r");
							} catch (FileNotFoundException e1) {
								e1.printStackTrace();
							}
							StringBuilder s = new StringBuilder();
							try {
								if(f.length()>=6)
								{
									for(int i =0; i<5; i++)
									{
										s.append((char)f.readByte());
										f.seek(i+1);
									}
								}
							} catch (IOException e1) {
								e1.printStackTrace();
							}
							try {
								f.close();
							} catch (IOException e1) {
								e1.printStackTrace();
							}
							int e;
							// Two variants of IPS, the normal PATCH type, then this weird one called IPS32 with messily hacked in 32 bit offsets
							if(s.toString().equals("IPS32"))
							{
								e = ips32PatchRom(Globals.fileToPatch, Globals.patchToApply);
								if(e!=0)
								{
									msg=parseError(e, Globals.TYPE_IPS);
								}
							}else{
								 e = ipsPatchRom(Globals.fileToPatch, Globals.patchToApply);
									if(e!=0)
									{
										msg=parseError(e, Globals.TYPE_IPS);
									}
							}
							
						}
						if(Globals.patchToApply.toLowerCase(Locale.US).endsWith(".ups")||Globals.patchToApply.toLowerCase().endsWith(".xdelta")||Globals.patchToApply.toLowerCase().endsWith(".bps")||Globals.patchToApply.toLowerCase().endsWith(".bsdiff"))
						{
							File oldrom = new File(Globals.fileToPatch);
							File bkrom = new File(Globals.fileToPatch+".bak");
							oldrom.renameTo(bkrom);
							File newrom = new File(Globals.fileToPatch+".new");
							newrom.renameTo(oldrom);
						}
						if(!c)
						{
							File f = new File(Globals.fileToPatch+".bak");
							if(f.exists())
							{
								f.delete();
							}
						}else{
							if(d)
							{
								File one = new File(Globals.fileToPatch+".bak");
								File two = new File(Globals.fileToPatch);
								File three = new File(Globals.fileToPatch.substring(0,Globals.fileToPatch.lastIndexOf('/')+1)+ed);
								two.renameTo(three);
								File four = new File(Globals.fileToPatch);
								one.renameTo(four);	
							}
						}
						Globals.msg=msg;
					}else{
						Globals.msg=getResources().getString(R.string.fnf);
					}
		        }
		    }).start();
			new Thread(new Runnable() {                 
				@Override
				public void run() {
				try
				   {
				    while(Globals.msg.equals(""));
				    myPd_ring.dismiss();
				    runOnUiThread(new Runnable() {
				        public void run() {
				        	Toast t = Toast.makeText(staticThis, Globals.msg, Toast.LENGTH_SHORT);
						    t.show();
				        }
				    });
				    Thread.sleep(3000); // Wait to clear Globals.msg
				    Globals.msg="";
				  }catch(Exception e){}
				 }
				 }).start();
	}
	
	@Override
	public boolean onKeyDown(int KeyCode, KeyEvent event) {
		if(KeyCode == KeyEvent.KEYCODE_BACK)
		{
			System.exit(0);

		}
		return true;
	}

	public String parseError(int e, int t)
	{
		switch(t)
		{
		case Globals.TYPE_UPS:
			switch(e)
			{
			case -1:
				return getResources().getString(R.string.upsNegativeOne);
			case -2:
				return getResources().getString(R.string.upsNegativeTwo);
			case -3:
				return getResources().getString(R.string.upsNegativeThree);
			case -4:
				return getResources().getString(R.string.upsNegativeFour);
			case -5:
				return getResources().getString(R.string.upsNegativeFive);
			case -6:
				return getResources().getString(R.string.upsNegativeSix);
			case -7:
				return getResources().getString(R.string.upsNegativeSeven);
			default:
				return getResources().getString(R.string.upsDefault)+e;
			}
		case Globals.TYPE_XDELTA:
			switch(e)
			{
			case -17710:
				return getResources().getString(R.string.xdeltaNegativeSeventeenThousandSevenHundredTen);
			case -17711:
				return getResources().getString(R.string.xdeltaNegativeSeventeenThousandSevenHundredEleven);
			case -17712:
				return getResources().getString(R.string.xdeltaNegativeSeventeenThousandSevenHundredTwelve);
			default:
				return getResources().getString(R.string.xdeltaDefault)+e;	
			}
		case Globals.TYPE_BPS:
			switch(e)
			{
			case 1:
				return getResources().getString(R.string.bpsOne);
			case -1:
				return getResources().getString(R.string.bpsNegativeOne);
			case -2:
				return getResources().getString(R.string.bpsNegativeTwo);
			case -3:
				return getResources().getString(R.string.bpsNegativeThree);
			case -4:
				return getResources().getString(R.string.bpsNegativeFour);
			case -5:
				return getResources().getString(R.string.bpsNegativeFive);
			case -6:
				return getResources().getString(R.string.bpsNegativeSix);
			case -7:
				return getResources().getString(R.string.bpsNegativeSeven);
			case -8:
				return getResources().getString(R.string.bpsNegativeEight)+e;
			default:
				return getResources().getString(R.string.bpsDefault)+e;
			}
		case Globals.TYPE_BSDIFF:
			switch(e)
			{
			case 1:
				return getResources().getString(R.string.bsdiffOne);
			case 2:
				return getResources().getString(R.string.bsdiffTwo);
			default:
				return getResources().getString(R.string.bsdiffDefault)+e;
			}
		case Globals.TYPE_PPF:
			switch(e)
			{
			case 1:
				return getResources().getString(R.string.ppfOne);
			case -1:
				return getResources().getString(R.string.ppfNegativeOne);
			case -2:
				return getResources().getString(R.string.ppfNegativeTwo);
			case -3:
				return getResources().getString(R.string.ppfNegativeThree);
			case -4:
				return getResources().getString(R.string.ppfNegativeFour);	
			default:
				return getResources().getString(R.string.ppfDefault)+e;
			}
			case Globals.TYPE_IPS:
				switch(e)
				{
				case 1:
					return getResources().getString(R.string.ipsOne);
				case -1:
					return getResources().getString(R.string.ipsNegativeOne);
				case -2:
					return getResources().getString(R.string.ipsNegativeTwo);
				case -3:
					return getResources().getString(R.string.ipsNegativeThree);
				case -4:
					return getResources().getString(R.string.ipsNegativeFour);
				case -5:
					return getResources().getString(R.string.ipsNegativeFive);
				case -6:
					return getResources().getString(R.string.ipsNegativeSix);
				case -7:
					return getResources().getString(R.string.ipsNegativeSeven);
				default:
					return getResources().getString(R.string.ipsDefault)+e;
				}
			case Globals.TYPE_IPS32:
				switch(e)
				{
				case 1:
					return getResources().getString(R.string.ips32One);
				case -1:
					return getResources().getString(R.string.ips32NegativeOne);
				case -2:
					return getResources().getString(R.string.ips32NegativeTwo);
				case -3:
					return getResources().getString(R.string.ips32NegativeThree);
				case -4:
					return getResources().getString(R.string.ips32NegativeFour);
				case -5:
					return getResources().getString(R.string.ips32NegativeFive);
				case -6:
					return getResources().getString(R.string.ips32NegativeSix);
				case -7:
					return getResources().getString(R.string.ips32NegativeSeven);
				default:
					return getResources().getString(R.string.ips32Default)+e;
				}
			default:
				return getResources().getString(R.string.errorDefault);
		}
	}
	
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		if (requestCode == 1) {
			if (resultCode == RESULT_OK) {
				TextView tv1 = (TextView) findViewById(R.id.romText);
				TextView tv2 = (TextView) findViewById(R.id.patchText);
				if(Globals.fileToPatch.lastIndexOf("/")>-1)
				{
					tv1.setText(Globals.fileToPatch.substring(Globals.fileToPatch.lastIndexOf("/")+1));
				}
				if(Globals.patchToApply.lastIndexOf("/")>-1)
				{
					tv2.setText(Globals.patchToApply.substring(Globals.patchToApply.lastIndexOf("/")+1));
				}
				tv1.postInvalidate();
				tv2.postInvalidate();
			}
		}
	}
}
