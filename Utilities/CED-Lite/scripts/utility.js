/******************************************************************************
 * Copyright 2012 Intel Corporation All Rights Reserved.
 *
 * The source code, information and material ("Material") contained herein is
 * owned by Intel Corporation or its suppliers or licensors, and title to such
 * Material remains with Intel Corporation or its suppliers or licensors. The 
 * Material contains proprietary information of Intel or its suppliers and 
 * licensors. The Material is protected by worldwide copyright laws and treaty 
 * provisions. No part of the Material may be used, copied, reproduced, modified,
 * published, uploaded, posted, transmitted, distributed or disclosed in any way 
 * without Intel's prior express written permission. No license under any patent,
 * copyright or other intellectual property rights in the Material is granted to 
 * or conferred upon you, either expressly, by implication, inducement, estoppel 
 * or otherwise. Any license under such intellectual property rights must be 
 * express and approved by Intel in writing.
 *
 * Unless otherwise agreed by Intel in writing, you may not remove or alter this
 * notice or any other notice embedded in Materials by Intel or Intel's suppliers 
 * or licensors in any way.
 *
 ********************************************************************************/

var ZERO = "0";

function saveStringToFile(str, defaultFilename) {
	var saveNewWindow = document.getElementById('radio_save_dest2').checked;
	
	if (saveNewWindow) {
		var winRef;
		
		if(navigator.appName.indexOf("Internet Explorer") > -1)  {
			winRef = window.open('about:blank', '');
			winRef.document.open('text/plain','replace');
			winRef.document.write(str);
		} else {
			winRef = window.open("data:text/plain;charset=utf-8," + encodeURIComponent(str), '');		
		}
	
		winRef.focus();
		winRef.document.close();
	
	} else {    
		if(navigator.appName.indexOf("Internet Explorer") > -1)  {        
			saveFileIE(str, defaultFilename);	
		} else {		 
		   saveFileFirefox(str, defaultFilename);
		}
	}
		
}

function saveFileFirefox(output, defaultFilename)
{
	try
	{
		netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');
	}
	catch(err)
	{
		alert("Unable to get save permission. Doing alternate save.\n\nRename saved file to " + defaultFilename);
		var winRef = window.open("data:application/octet-stream," + encodeURIComponent(output), '');
		return;
	}

	var filePickerInterface = Components.interfaces.nsIFilePicker;
	var filePickerInstance = Components.classes["@mozilla.org/filepicker;1"].createInstance(filePickerInterface);

	filePickerInstance.init(window, "Save File...", filePickerInterface.modeSave);
	filePickerInstance.appendFilter("Text File","*.txt; *.doc; *.rtf");
	filePickerInstance.defaultString=defaultFilename;
	
	var rv = filePickerInstance.show();
	
	if (rv == filePickerInstance.returnCancel)
		return;	
	else if(rv == filePickerInterface.returnOK || rv == filePickerInterface.returnReplace)
	{
	    var file = filePickerInstance.file;

		if(!file.exists())
		{
			file.create( Components.interfaces.nsIFile.NORMAL_FILE_TYPE, 420 );
		}
		
		var ofStream = Components.classes["@mozilla.org/network/file-output-stream;1"].createInstance( Components.interfaces.nsIFileOutputStream );
		ofStream.init( file, 0x04 | 0x08 | 0x20, 640, 0 );
		ofStream.write( output, output.length );
		ofStream.close();
		alert('File Save Successful' );
	}
}

function saveFileIE(output,defaultFilename)
{
    var windowFrame = window.frames.save;
	
    if( !windowFrame )
	{
        var frame = document.createElement( 'iframe' );
        frame.id = 'save';
        frame.style.display = 'none';
        document.body.insertBefore( frame );
        windowFrame = window.frames.save;
    }
    
    var doc = windowFrame.document;
    doc.open( 'text/Plain', 'replace');
    doc.charset = "UTF-8";
    
    doc.write(output);
    doc.close();	
        
    if( doc.execCommand( 'SaveAs', false , defaultFilename ) )
    {
        alert('File saved successfully.' );
    }
    else
    {
        alert( 'File save was unsuccessful.' );
    }
	
    windowFrame.close();
}


function getFileAsString(fileBrowseObject, callback) {
	var pathName = fileBrowseObject.value;
	var strURL;

    if (fileBrowseObject.files) {
        var reader = new FileReader();
        reader.onload = function(evt) {
			strURL = evt.target.result;
			callback(strURL);
		};
		reader.readAsText(fileBrowseObject.files.item(0));				
    } else { // for IE        
        callback(ieReadFile(pathName));		
    }	
}

function ieReadFile(filename) 
{
    try
    {
        var fso  = new ActiveXObject("Scripting.FileSystemObject"); 
        var fh = fso.OpenTextFile(filename, 1); 
        var contents = fh.ReadAll(); 
        fh.Close();
        return contents;
    }
    catch (exception)
    {
		if (exception.message == "Automation server can't create object")		
			alert("Your browser security settings are preventing uploading files.\nEither switch to Firefox, download the site for offline use and run locally, or adjust the security settings of IE");
		else
			alert("Cannot open file");
			
        return null;
    }
}



 function processBrightContrastAttr(red, green, blue, brightness)  {
    var color;
      if (red == "" && green =="" && blue == "")
      {
         return null;
      }
      
      color = "0x";
      if(red == "")
      {
         color = color+computeBrightContrast("0", true);
      }
      else
      {
         color = color+computeBrightContrast(red,true);
      }
      if(green == "")
      {
         color = color+computeBrightContrast("0", true);
      }
      else
      {
        color = color+computeBrightContrast(green,true);
      }
      if(blue == "")
      {
         color = color+computeBrightContrast("0", true);
      }
      else
      {
         color = color+computeBrightContrast(blue,true);
      }
      var attrid = 37;
      if(brightness)
      {
	attrid = 36;
      }
      return [attrid, color];
   }

function processGammaAttr(gammar, gammag, gammab)
   {
    var gamma;
      if (gammar=="" && gammag=="" && gammab=="")
      {
         return null;
      }
      gamma = "0x";
      if(gammar == "")
      {
         gamma = gamma+"0"+"0";
      }
      else
      {
         gamma = gamma+computeGammaHex(gammar);
      }
      if(gammag == "")
      {
          gamma = gamma+"0"+"0";
      }
      else
      {
         gamma = gamma+computeGammaHex(gammag);
      }
      if(gammab == "")
      {
          gamma = gamma+"0"+"0";
      }
      else
      {
         gamma = gamma+computeGammaHex(gammab);
      }
      var attrid = 35;
      return [attrid, gamma];
   }

function computeBrightContrast(userValue, isAttribute)
   {
     var intVal = parseInt(userValue);
      if(isAttribute)
      {
         var hexVal = parseInt(intVal + 128).toString(16);
         if(hexVal.length==1)
         {
            hexVal = "0"+hexVal;
         }
         return hexVal;
      }
      intVal = intVal * 65535 / 200;
      if(intVal < 65535)
      {
         intVal++;
      }
	  intVal = parseInt(intVal);
      return "0x"+intVal.toString(16);
   }

function computeGammaHex(value)
{
    var bin3i="";
      var bin5f="";
      var gammaValue = (value+"");
      var decimal = gammaValue.indexOf(".");
      if(decimal >= 0)
      {
         if(decimal == 0)
         {
            bin3i = "0";
         }
         else
         {
            bin3i = parseInt(gammaValue.substring(0,decimal)).toString(2);
         }
         //calulate 5f value
         var fixedPt;
         if(gammaValue.substring(decimal).length>1)
         {
            fixedPt = Number(gammaValue.substring(decimal));
         }
         else
         {
            fixedPt = 0.0;;
         } 
         bin5f = fixedPoint(5,fixedPt);
      }
      else
      {
         bin3i = parseInt(gammaValue).toString(2);
      }
      while(bin5f.length<5)
      {
         bin5f = "0"+bin5f;
      }
      var hex3i5f = parseInt((bin3i + bin5f),2).toString(16);
      if(hex3i5f.length<2)
      {
         hex3i5f = "0"+hex3i5f;
      }
      return hex3i5f;
}

function processOverlayGamma(gammaValue)
{
   //special case .6 24i8f = 0x99 but for hardware smallest value should be 0x9a
   if(gammaValue == 0.6)
   {
	  return ("0x0000009a");
   }
   var gammaValueStr = gammaValue+"";
   //create a 24i8f number from the gammaValue
   var bin24i = "";
   var bin8f = "";
   var decimal = gammaValueStr.indexOf(".");

   if(decimal >= 0)
   {
	  if(decimal != 0)
	  {
		bin24i = (parseInt(gammaValueStr.substring(0,decimal))).toBinaryString();
	  }
	  //calulate 8f
	  var fixedPt;
	  if(gammaValueStr.substring(decimal,gammaValueStr.length).length>1)
	  {
		 fixedPt = parseFloat(gammaValueStr.substring(decimal,gammaValueStr.length));
	  }
	  else
	  {
		 fixedPt = 0.0;
	  }
	  bin8f = fixedPoint(8,fixedPt);
   }
   else
   {
	  bin24i = (parseInt(gammaValueStr)).toBinaryString();
   }
   
   while(bin8f.length<8)
   {
	  bin8f = ZERO+bin8f;
   }
   
   var hex24i8f = (parseInt((bin24i + bin8f),2)).toString(16);
   while(hex24i8f.length<8)
   {
	  hex24i8f = ZERO+hex24i8f;
   }
   
   return "0x" + hex24i8f;
}

function fixedPoint(floatPt, decimal)
   {
    var bin = "";
      var decimalPoint;
      for(i=0;i<floatPt;i++)
      {
         decimal = parseFloat(decimal * 2);
         if(decimal>=1)
         {
			decimalStr = decimal+"";
            decimalPoint = decimalStr.indexOf(".");
			if(decimalPoint !== -1)			
				decimal = Number(decimalStr.substring(decimalPoint));
			else
				decimal = 0;
            bin = bin+"1";
         }
         else
         {
            bin = bin+"0";
         }
      }
      return bin;
   }