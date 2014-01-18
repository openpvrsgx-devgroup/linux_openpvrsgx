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

$(function() {
	$( "#dialog-save-form" ).dialog({
		autoOpen: false,
		height: 280,
		width: 500, 
		modal: true,
		buttons: {
			"Save in Browser": function() {				
				var title = document.getElementById("dialog_saveTitle");						
				saveSelections(title.value, "local");
				$( this ).dialog( "close" );					
			},
			"Save to File": function() {
				var title = document.getElementById("dialog_saveTitle");						
				saveSelections(title.value, "external");				
				$( this ).dialog( "close" );							
			},
			Cancel: function() {
				$( this ).dialog( "close" );
			}
		},
		close: function() {					
		}
	});
});

$(function() {
	$( "#dialog-load-form" ).dialog({
		autoOpen: false,
		autoHeight: false,
		//height: 280,
		width: 600, 
		modal: true,
		buttons: {
			"Load": function() {	
				var r = document.load_form.loadState_radio;
				if (document.getElementById("loadState_radio_External").checked) {
					function callback(fileStr)
                    {
                        loadSelections(fileStr);
                    }
                    getFileAsString(document.load_form.loadState_uploadFile, callback);					
				} else {
					for (var i=0; i<r.length-1; i++) {
						if (r[i].checked)
							loadSaveByTitle(r[i].value);
					}						
				}
				$( this ).dialog( "close" );					
			},
			Cancel: function() {
				$( this ).dialog( "close" );
			},
			Delete: function() {
				var r = document.load_form.loadState_radio;				
				if (document.getElementById("loadState_radio_External").checked)
					alert("Select a local save configuration to delete");
				else {
					for (var i=0; i<r.length-1; i++) {
						if (r[i].checked)
							deleteSave(r[i].value);
					}						
					$( this ).dialog( "close" );
				}				
			}
		},
		close: function() {					
		}
	});
});				

var cookiePrefix = "CEDLite_";

function addPrefixToSaveTitle(saveTitle) {	
	if (saveTitle != null && saveTitle.length > 0)
		var saveTitleNoSpaces = saveTitle.split(' ').join('_');
	else
		var saveTitleNoSpaces = 'default';
		
	return cookiePrefix + saveTitleNoSpaces;
}

function saveSelections(saveTitle, saveType) {	
	var frm = document.f1;
	var formname = frm.name;
	var i;
	
	var cookieName = addPrefixToSaveTitle(saveTitle);

	// Expire cookie in 999 days.
	var today = new Date();
	var exp   = new Date(today.getTime()+999*24*60*60*1000);

	var string = ""; 
	
	for (i = 0; i < frm.elements.length; i++)	{
		
		if (frm.elements[frm[i].name] == null)
			continue;				
		
		string = string + getElementString(frm[i]);
	}
	
	/* Save any selection in the options form  */
	var frm2 = document.getElementById('options_form');
	if (frm2 != null) {
		for (i=0; i<frm2.elements.length; i++) {
			if (frm2.elements[frm2[i].name] == null)
				continue;				
			
			string = string + getElementString(frm2[i]);
		}
	}

	if (saveType == "external") {	
		saveStringToFile(string, "CEDLite_"+saveTitle+".txt");
		//alert('Config text written to new window. Save the text. To load, select "Load external" and paste in the saved text');
	}
	else {
		setCookie(cookieName, string, exp); 		
		//alert("Save complete");
	}
		
	
}

function getElementString(element)
{	
	// TEXT, TEXTAREA, and DROPDOWN
	if ( element.type == "text" || element.type == "textarea" || element.type == "select-one" || element.type == "hidden")	{				
		if (element.value != null && element.value.length > 0) {				
			return element.id+"="+element.value + "\|";					
		}
	} 

	// CHECKBOX and RADIO
	if (element.type == "checkbox" || element.type == "radio"){		
		if (element.checked==true) {					
			return element.id+"=1\|";
		}
	}				

	return "";
}

//
// LOAD FORM FIELD SELECTIONS FROM SAVED COOKIES
//

function loadSaveByTitle(saveTitle)
{
	var	fieldValues = getCookie(cookiePrefix+saveTitle);
	loadSelections(fieldValues);
}

function loadSelections(fieldValues) {
	var frm = document.f1;
	var fieldArray;	
	var attrArray = new Array();
	var attrValueArray = new Array();
	var i, eID, val;
	var validInput = false;

	if (fieldValues == null) {
		alert("unable to load saved settings");
		return;
	}
		
	frm.reset();  // reset form values to default
	$('input[type="checkbox"]:checked').each( 
						function() {
							$(this).attr("checked", false);
						} 
					);

	fieldArray  = fieldValues.split("\|");	
	
	for (i=0; i<fieldArray.length; i++) {
		fieldStrings = fieldArray[i].split("=");		
		if (fieldStrings.length != 2) {
		 continue;
		}
		
		eID = fieldStrings[0];
		val = fieldStrings[1];
				
		/* Attributes are disabled until SDVO cards are set, so save for later */		
		if (eID.indexOf("_attr_") >= 0) {
			if (eID.indexOf("_custom_chkbox_") >= 0){
				var aSplit = eID.split("_");
				addCustomAttributeByPort(aSplit[0], aSplit[aSplit.length-1]);
			}
			attrArray.push(eID);
			attrValueArray.push(val);
			continue;
		}
		
		element = document.getElementById(eID);
		if (element == null) {
			alert("Unable to find element id: " + eID);
			continue;
		}
		validInput = true;
		setElement(element, val);
	}
	
	for (i=0; i<attrArray.length; i++)	{	
		element = document.getElementById(attrArray[i]);
		if (element == null) {
			alert("Unable to find element id: " + attrArray[i]);
			continue;
		}				
		validInput = true;
		setElement(element, attrValueArray[i]);
	}
	
	setColorCorrectionSlider();
	
	if (validInput == false) {
		alert("Invalid load file");
	}	
	
}

function setElement(element, val)
{	
var functionObj = null;

	if (element.type == "checkbox" || element.type == "radio") {				
		if (val == "1") {
			element.checked = true;
			
			/* If checkbox has onclick function */
			functionObj = document.getElementById(element.id).getAttributeNode('onclick');						
		}
	}	
	else if ( element.type == "text" || element.type == "textarea" || element.type == "select-one" )	{
		element.value = val;		    	
		
		/* if combobox has onchange function */
		functionObj = document.getElementById(element.id).getAttributeNode('onchange');							
	}
	
	/* Run onclick or onchange function for this control */
	if(functionObj != null && functionObj.nodeValue != null)	{
		if((functionObj.nodeValue+"").indexOf("event") > -1) {
			var event = new Object();
			event.target = element;							
			window[(functionObj.nodeValue+"").substring(0,functionObj.nodeValue.indexOf('('))](event);
		} else
			window[(functionObj.nodeValue+"").substring(0,functionObj.nodeValue.indexOf('('))]();
	}
}

function deleteSave(saveTitle) {		
	setCookie(addPrefixToSaveTitle(saveTitle), "", new Date()); 
}

function setCookie(name, value, expires, path, domain, secure) {
	document.cookie= name + "=" + escape(value);
	if(expires)
		document.cookie += "; expires=" + expires.toGMTString();
	if(path)
		 document.cookie += "; path=" + path;
	if(domain)
		document.cookie += "; domain=" + domain;
	if(secure)
		document.cookie += "; secure";
}

function getCookie(name) {
	var cookie = document.cookie;	
	var prefix = name + "=";
	
	var begin = cookie.indexOf(prefix)!=-1?cookie.indexOf(prefix):null;	
	
	var end = cookie.indexOf(";", begin)!=-1?cookie.indexOf(";", begin):cookie.length;
	
	return unescape(cookie.substring(begin + prefix.length, end));
}

function getSavedCookies()
{
	var dc = document.cookie;
	var arrayOfCookies = dc.split(";");
	var pair;
	var result = new Array();
	var foundCookie = false;
	
	for (var i=0; i < arrayOfCookies.length; i++){
		pair = arrayOfCookies[i].split("=");		
		if (pair.length >= 2) {
			// Must contain CEDLite prefix		
			var prefix = pair[0].indexOf(cookiePrefix);			
			if (prefix >= 0) {
				var name = pair[0].substring(prefix+cookiePrefix.length, pair[0].length);			
				result[name] = unescape(pair[1]);
				foundCookie = true;
			}
		}		
	}
		
	if (foundCookie == false)
		return null;
	
	return result;
}
