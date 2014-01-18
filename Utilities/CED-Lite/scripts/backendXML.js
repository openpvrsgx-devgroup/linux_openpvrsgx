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

function openControlsXML()
{
	var xmlHttpRequest;
	var xmlDoc;
	if (window.XMLHttpRequest && navigator.appName.indexOf("Internet Explorer") == -1) {
		xmlHttpRequest=new XMLHttpRequest();
	} else{
		xmlHttpRequest=new ActiveXObject("Microsoft.XMLHTTP");
    }
	xmlHttpRequest.open("GET","xml/controls.xml",false);
	xmlHttpRequest.send();
	var txt = xmlHttpRequest.responseText;
	if (window.DOMParser) 	{
		parser=new DOMParser();
		xmlDoc=parser.parseFromString(txt,"text/xml");
	}
	else { // Internet Explorer
		xmlDoc=new ActiveXObject("Microsoft.XMLDOM");
		xmlDoc.async="false";
		xmlDoc.loadXML(txt); 
	}
       
	return xmlDoc;
}

function openDTDXML(name)
{
	if (name.indexOf("Custom ") >= 0) {		
		return getXMLDoc(customDTDs[name]);
	}
		
	var xmlHttpRequest;
	if (window.XMLHttpRequest && navigator.appName.indexOf("Internet Explorer") == -1) {
		xmlHttpRequest=new XMLHttpRequest();
	} 
	else {
		xmlHttpRequest=new ActiveXObject("Microsoft.XMLHTTP");
	}
	var fileName = "dtd/"+name+".dtd";
	xmlHttpRequest.open("GET",fileName,false);
	xmlHttpRequest.send();
	var txt = xmlHttpRequest.responseText;
  
	return getXMLDoc(txt);
}

function getXMLDoc(txt)
{
	if (window.DOMParser)
	{
		parser=new DOMParser();
		xmlDoc=parser.parseFromString(txt,"text/xml");
	}
	else // Internet Explorer
	{
	  xmlDoc=new ActiveXObject("Microsoft.XMLDOM");
	  xmlDoc.async="false";
	  xmlDoc.loadXML(txt); 
	}
return xmlDoc
}
//create a new DTD XML file with the appropriate headers.
function createDTDXML()
{
	var xmlTxt = '<?xml version="1.0" encoding="UTF-8"?>';
	xmlTxt = xmlTxt + '<cedSnapshot xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" type="DTD" xsi:type="cedSnapshot">';
    xmlTxt = xmlTxt + '<Selections></Selections></cedSnapshot>';
	if (window.DOMParser)
	{
		xmlDom = new DOMParser();
		xmldoc = xmlDom.parseFromString(xmlTxt,"text/xml");
	}
	else // Internet Explorer
	{
	  xmldoc=new ActiveXObject("Microsoft.XMLDOM");
	  xmldoc.async="false";
	  xmldoc.loadXML(xmlTxt); 
	}
	return xmldoc;
}

//create an element in the DTD XML
function createDTDSelection(xmldoc, item_type_key, entry_value, entry_name, entry_id)
{
	newel=xmldoc.createElement("selection");
	x=xmldoc.getElementsByTagName("Selections")[0];
	
	
	newatt1=xmldoc.createAttribute("page_name");
	newatt1.nodeValue="DTDPage";
	newatt2=xmldoc.createAttribute("item_type_key");
	newatt2.nodeValue=item_type_key;

	newel.setAttributeNode(newatt1);
	newel.setAttributeNode(newatt2);
	
	newSelEntry=xmldoc.createElement("selection_entry");

	if(entry_value != null && entry_value != "")
	{
		newatt1=xmldoc.createAttribute("entry_value");
		newatt1.nodeValue=entry_value;
		newSelEntry.setAttributeNode(newatt1);
	}
	if(entry_name != null && entry_name != "")
	{
		newatt2=xmldoc.createAttribute("entry_name");
		newatt2.nodeValue=entry_name;
		newSelEntry.setAttributeNode(newatt2);
	}
	if(entry_id != null && entry_id != "")
	{
		newatt3=xmldoc.createAttribute("entry_id");
		newatt3.nodeValue=entry_id;
		newSelEntry.setAttributeNode(newatt3);
	}
	newel.appendChild(newSelEntry);
	
	x.appendChild(newel);
}
function constructDTD()
{
	var xmldoc = createDTDXML();
	var mainForm = document.f1;
	dtdTranslation("emgd", mainForm["dtd_type"].value);
	
	createDTDSelection(xmldoc, "dtd_name", mainForm["dtd_name"].value, mainForm["dtd_name"].value, null);
	createDTDSelection(xmldoc, "dtd_type", mainForm["dtd_type"].selectedIndex+1, mainForm["dtd_type"].options[mainForm["dtd_type"].selectedIndex].text, mainForm["dtd_type"].selectedIndex+1);
	
	$("input.emgd_parameters").each(function(){
		
		if(mainForm[$(this).attr("id")].type == "checkbox")
		{
			createDTDSelection(xmldoc, $(this).attr("id"), mainForm[$(this).attr("id")].getSelection(), mainForm[$(this).attr("id")].checked?"true":"false", null);
		}
		else if(isset(mainForm[$(this).attr("id")]))
			createDTDSelection(xmldoc, $(this).attr("id"), mainForm[$(this).attr("id")].getSelection(), mainForm[$(this).attr("id")].value, null);
		
	});
	
	$("select.emgd_parameters").each(function(){
			createDTDSelection(xmldoc, $(this).attr("id"), mainForm[$(this).attr("id")].getSelection(), mainForm[$(this).attr("id")].options[mainForm[$(this).attr("id")].selectedIndex].text, null);
	});
		
	if(navigator.appName.indexOf("Internet Explorer") > -1)
    {
		xmlstr = xmldoc.xml;
	}
	else
	{
		xmlstr = (new XMLSerializer()).serializeToString(xmldoc);
	}
	
	return xmlstr;
}

function exportDTD() {
	var mainForm = document.f1;
	var str=constructDTD();
	saveStringToFile(str, mainForm["dtd_name"].value+".xml");    
}

function addDTD()
{
	addDTDToList(document.getElementById("dtd_name").value, constructDTD());
	document.getElementById("dtd_added_label").innerHTML = "New Custom DTD added to DTD List";
}