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

function processMainTemplate() {
	var fh;
	if (window.XMLHttpRequest && navigator.appName.indexOf("Internet Explorer") == -1) {
		fh=new XMLHttpRequest();
	}
	else {
		fh=new ActiveXObject("Microsoft.XMLHTTP");
	}
	fh.open('GET', 'xTemplate/main_template_linux', false);
	fh.send();
	var str = fh.responseText;	
			
	str = str.replace("@START_CONFIG", processConfigTemplate("primary"));

	if (configTemplate["display_config_mode"] == "8")	{
		str = str.replace('@DIH_INFO', processDIHTemplate());
		str = str.replace('@DIH_SCREEN', '    Screen 1       "Screen1" RightOf "Screen0"');
	}

	for (var k in mainTemplate)	{		 			
		rep="\\"+special+k+"\\"+special;	
		str = replaceAll(rep, mainTemplate[k], str);				 
	}

	for (var k in sharedTemplate){		 			
		rep="\\"+special+k+"\\"+special;	
		str = replaceAll(rep, sharedTemplate[k], str);				 
	}

	str = removeUnusedTags(str);
	if(!testing)
		saveStringToFile(str, "xorg.txt");
	else
	{
		return str;
	}
}

function processConfigTemplate(display) {
	var fh;
	if (window.XMLHttpRequest && navigator.appName.indexOf("Internet Explorer") == -1)	{
		fh=new XMLHttpRequest();
	} 
	else {
		fh=new ActiveXObject("Microsoft.XMLHTTP");
	}
	fh.open('GET', 'xTemplate/config_template_linux', false);
	fh.send();
	var str = fh.responseText;	
			
	if (display != "secondary")	{
		str = str.replace('@START_PORT', processPortTemplate());
	}			

	for (var k in configTemplate)	{		 			
		rep="\\"+special+k+"\\"+special;	
		str = replaceAll(rep, configTemplate[k], str);				 
	}		
	return str;
}

function processPortTemplate() {
	var str="";
	for (var p in port)	{
		str += processPortHelper(port[p]);
	}	 
	return str;
}

function processPortHelper(portTemplate) {
	var fh;
	if (window.XMLHttpRequest && navigator.appName.indexOf("Internet Explorer") == -1)	{
		fh=new XMLHttpRequest();
	}
	else {
		fh=new ActiveXObject("Microsoft.XMLHTTP");
	}
	fh.open('GET', 'xTemplate/port_template_linux', false);
	fh.send();
	var str = fh.responseText;	

	if (portTemplate['dtd'] != null) { 	  
		str = str.replace('@START_DTD', processDTDTemplate(portTemplate['PORT_ID'], portTemplate['dtd']));
	} 
		
	if (portTemplate['attr'] != null) {
		str = str.replace('@START_ATTR', processAttrTemplate(portTemplate['PORT_ID'], portTemplate['attr']));
	}

	for (var k in portTemplate)	{		 		
		rep="\\"+special+k+"\\"+special;	
		str = replaceAll(rep, portTemplate[k], str);				 
	}		
	return str;
}

function processDTDTemplate(PORT_ID, dtd) {
	var str="";
	if (dtd != null) {
		for (var d in dtd)	{
			str += processDTDHelper(PORT_ID, dtd[d]);
		}	 
	}
	return str;
}

function processDTDHelper(PORT_ID, dtdTemplate) {
	var fh;
	if (window.XMLHttpRequest && navigator.appName.indexOf("Internet Explorer") == -1)	{
		fh=new XMLHttpRequest();
	}
	else {
		fh=new ActiveXObject("Microsoft.XMLHTTP");
	}
	fh.open('GET', 'xTemplate/dtd_template_linux', false);
	fh.send();
	var str = fh.responseText;	

	for (var k in dtdTemplate)	{		 			
		rep="\\"+special+k+"\\"+special;	
		str = replaceAll(rep, dtdTemplate[k], str);				 
	}		
	return str;
}

function processAttrTemplate(PORT_ID, attr) {
	var str="";
	for (var a in attr)	{
		str += processAttrHelper(attr[a]);
	}	 
	return str;
}

function processAttrHelper(attrTemplate) {
	var fh;
	if (window.XMLHttpRequest && navigator.appName.indexOf("Internet Explorer") == -1)	{
		fh=new XMLHttpRequest();
	}
	else {
		fh=new ActiveXObject("Microsoft.XMLHTTP");
	}
	fh.open('GET', 'xTemplate/attr_template_linux', false);
	fh.send();
	var str = fh.responseText;	

	for (var k in attrTemplate)	{		 			
		rep="\\"+special+k+"\\"+special;	
		str = replaceAll(rep, attrTemplate[k], str);
		str = str.replace(/\s+$/,'');
		str = str+"\n";
	}		
	return str;
}

function processDIHTemplate() {	
	if (configTemplate["display_config_mode"] != "8")
		return special;  /* return special so the whole line gets removed later */

	var fh;
	if (window.XMLHttpRequest && navigator.appName.indexOf("Internet Explorer") == -1)	{
		fh=new XMLHttpRequest();
	}
	else {
		fh=new ActiveXObject("Microsoft.XMLHTTP");
	}
	fh.open('GET', 'xTemplate/dih_template_linux', false);
	fh.send();
	var str = fh.responseText;	

	str = str.replace('@START_CONFIG', processConfigTemplate("secondary"));
	str = str.replace('@XINERAMA', processXineramaTemplate());

	for (var k in mainTemplate)	{		 			
		rep="\\"+special+k+"\\"+special;	
		str = replaceAll(rep, mainTemplate[k], str);				 
	}		
	return str;
}

function processXineramaTemplate()
{
	var fh;
	if (window.XMLHttpRequest && navigator.appName.indexOf("Internet Explorer") == -1)	{
		fh=new XMLHttpRequest();
	}
	else {
		fh=new ActiveXObject("Microsoft.XMLHTTP");
	}
	fh.open('GET', 'xTemplate/xinerama_template_linux', false);
	fh.send();
	var str = fh.responseText;	

	str = str.replace('True', configTemplate["xinerama"]);
	return str;
}

function replaceAll(replaceThis, withThis, string) {
	var re = new RegExp(replaceThis, "g");
	return string.replace(re, withThis);
}

function removeUnusedTags(str)
{
	var result = "";
	lineArray = str.split("\n");
	for (var i in lineArray) {
		if (lineArray[i].indexOf(special) < 0 && lineArray[i].indexOf('@') < 0)
			result += lineArray[i] + "\n";
	}	
	return result;
}
