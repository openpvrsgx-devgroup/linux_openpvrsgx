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
	$( "#dialog-options-form" ).dialog({
		autoOpen: false,
		autoHeight: false,
		//height: 280,
		width: 500, 
		modal: true,
		buttons: {	
			Close: function() {
				$( this ).dialog( "close" );
			}
		},
		close: function() {					
		}
	});
});				

//slider code
$(function() {
		$( ".gamma_slider" ).slider({
			orientation: "horizontal",
			range: "min",
			min: 0.6,
			max: 6,
			step: 0.1,
			value: 1,
			slide: function( event, ui ) {
			  if (!event) 
				event = window.event;
			
			  var control = event.target;			  			  
			  $( "#"+control.id.substr(0, control.id.lastIndexOf("_"))).val( ui.value );
			}
		});
		$( ".brightness_slider" ).slider({
			orientation: "horizontal",
			range: "min",
			min: -127,
			max: 127,
			step: 1,
			value: 0,
			slide: function( event, ui ) {
			  if (!event) 
				event = window.event;
			
			  var control = event.target;
			  $( "#"+control.id.substr(0, control.id.lastIndexOf("_"))).val( ui.value );
			}
		});
		$( ".other_slider" ).slider({
			orientation: "horizontal",
			range: "min",
			min: 0,
			max: 200,
			step: 1,
			value: 0,
			slide: function( event, ui ) {
			  if (!event) 
				event = window.event;
			
			  var control = event.target;
			  $( "#"+control.id.substr(0, control.id.lastIndexOf("_"))).val( ui.value );
			}
		});
	});
//end slider code

$(function() {
	$( ".closedPanel" ).accordion({
		collapsible: true,
		active: false,			
		autoHeight: false				
	});
});

$(function() {
	$( ".openPanel" ).accordion({
		collapsible: true,				
		autoHeight: false
	});
});

/* Set all input buttons in the jquery theme */
$(function() {
	$( "input:button" ).button();			
}); 

$(function() {
	$( ".help_icon" ).button({
		icons: {
                primary: "ui-icon-help"
		},
		text: false
	});			
}); 
	
$(function() {
	$( "#dialog-validate-form" ).dialog({
		autoOpen: false,
		modal: true,	
		minHeight: 250,
		buttons: {
			Ok: function() {
				$( this ).dialog( "close" );
			}
		}
	});
});

/* set settingstabs class to jquery tabs */
$(function(){  $('.settingstabs').tabs();  });

$(function() {
	$( ".jradio" ).buttonset();
});

function setColorCorrectionSlider()
{
	$(".gamma_slider").each(function()
							{
								var value = parseFloat(document.getElementById(($(this).attr("id")+"").substring(0, ($(this).attr("id")+"").lastIndexOf("_"))).value);
								if(isNaN(value) === false)
									$(this).slider("option", "value", value);
								else
									$(this).slider("option", "value", 1.0);
							});
	$(".brightness_slider").each(function()
							{
								var value = parseFloat(document.getElementById(($(this).attr("id")+"").substring(0, ($(this).attr("id")+"").lastIndexOf("_"))).value);
								if(isNaN(value) === false)
									$(this).slider("option", "value", value);
								else
									$(this).slider("option", "value", 0);
							});
	$(".other_slider").each(function()
							{
								var value = parseFloat(document.getElementById(($(this).attr("id")+"").substring(0, ($(this).attr("id")+"").lastIndexOf("_"))).value);
								if(isNaN(value) === false)
									$(this).slider("option", "value", value);
								else
									$(this).slider("option", "value", 0);
							});
}

function SetDisplayImage()
{
	len = document.f1.global_radio.length

	var imageElement = document.getElementById("secondDisplay");
	if(document.f1.global_radio[0].checked)	{
		imageElement.style.display = "none";
	}
	else {
		for (i = 0; i <len; i++)
		{
			if (document.f1.global_radio[i].checked) {
				imageElement.src = document.f1.global_radio[i].value;
				imageElement.style.display ="";
			}
		}
	}
	setPortVisibility();
}

function setPortVisibility()
{

	if (document.f1.global_radio[0].checked) {
		var primaryDisplay = document.getElementById("global_cbo_primary_display");
		var SelValue = primaryDisplay.options[primaryDisplay.selectedIndex].value;

		if (SelValue == "LVDS") {
			document.getElementById("lvds_panel").style.display="block"
			document.getElementById("sdvo1_panel").style.display="none"
		} else {
			document.getElementById("lvds_panel").style.display="none"
			document.getElementById("sdvo1_panel").style.display="block"
		}
	} else {
		document.getElementById("lvds_panel").style.display="block"
		document.getElementById("sdvo1_panel").style.display="block"
	}

	if (document.f1.global_radio[1].checked) {
		document.getElementById("field_clone_settings").style.display="block"
	} else  {
		document.getElementById("field_clone_settings").style.display="none"
	}
}

function fillPanels()
{
 document.getElementById('lvds_portsettings_panel').innerHTML=portSettingsTab('lvds');
 document.getElementById('sdvo1_portsettings_panel').innerHTML=portSettingsTab('sdvo'); 
}

function populateLoadForm()
{
	var cookieArray = getSavedCookies();			
	var tID = document.getElementById('dialog-load-table');
	
	// erase innerHTML and rebuild
	tID.innerHTML = ""; 
		
	var radioDefault = 'checked="yes"';
	if (cookieArray == null) {			
		tID.innerHTML += 'No browser saves found<br>';		
	} else {	
		for(var cName in cookieArray) {		
			tID.innerHTML += '<input type=radio name="loadState_radio" '+radioDefault+' Value="'+cName+'" id="loadState_radio_'+cName+
				'"><label for="loadState_radio_'+cName+'">'+cName+'</label><br>';				
			radioDefault = "";
		}
	}
	
	tID.innerHTML += '<br><input type=radio name="loadState_radio" '+radioDefault+' Value="External" id="loadState_radio_External"><label for="loadState_radio_External">Load from file</label><br>' +
		'<INPUT TYPE="file" NAME="loadState_uploadFile" id="loadState_uploadFile" size="40">';
		//'<input type="text" id="loadState_text_External" name="loadState_text_External">';  //size="10";

}

function importDTD()
{
	var browseObject = document.f1.dtdUploadFile;
	getFileAsString(browseObject, callback);
	
	function callback(fileContents)
	{
		if (fileContents == null)
		return;
	
		if (fileContents.indexOf('cedSnapshot') < 0) {
			alert("Invalid DTD XML file");
			return;
		}
		
		var fileName = browseObject.value.split('\\').pop().split('/').pop();
		
		addDTDToList(fileName, fileContents);
	}
}

function editDTD()
{
	var mainForm = document.f1;
	var filePath = mainForm["dtdUploadFile"];
	var data = getFileAsString(filePath, callback);
	
	function callback(data)
    {
            $(data).find("selection").each(function()
            {
                    itemType = $(this).attr("item_type_key")+"";
                    itemValue = $(this).find("selection_entry").attr("entry_value");
               
                    mainForm[itemType].setSelection(itemValue);
            });
               
            $('#general_tabs').tabs('select', 3);
            processDTDControls(mainForm["dtd_type"].value);
    }
}

function addDTDToList(dtdName, dtdContents)
{
	var htmlCtl;

	document.getElementById('addedDTDs').innerHTML += dtdName+"<br>";
	
	var ctrlName = "Custom " + dtdName;
	
	customDTDs[ctrlName] = dtdContents;	
	
	htmlCtl = "<input type='checkbox' name='lvds_dtds' value='"+ctrlName+"'>"+ctrlName+"<br>";
	document.getElementById("lvds_dtds").innerHTML = htmlCtl + document.getElementById("lvds_dtds").innerHTML;
	
	htmlCtl = "<input type='checkbox' name='sdvo_dtds' value='"+ctrlName+"'>"+ctrlName+"<br>";
	document.getElementById("sdvo_dtds").innerHTML = htmlCtl + document.getElementById("sdvo_dtds").innerHTML;	
}

function displayAbout()
{
	var about = "";
	$( '#dialog-options-form' ).dialog( 'close' );
	$.ajax({url: "xml/about.xml",
		dataType: "xml",async: false,
		success: function(data)
		{
			about = $(data).find("content").text().replace(/^\s+|\s+$/g, '');
			about = about+$(data).find("version").text();
		}
	});
	alert(about);
}

function portSettingsTab(prefix)
{
 str = "<table border='0' width='100%'>								"+
		"		<tr>									"+
		"		<td width='300px'>						"+
		"			<p>									";
		
		if(prefix == "lvds")
		{
			str +="Encoder <select id='lvds_encoder' name='lvds_encoder' onChange='setLVDSEncoder(event)'>"+
			"<option value='Internal LVDS'>Internal LVDS</option>"+
			"<option value='Chrontel* CH7036'>Chrontel* CH7036</option>"+
			"</select><br>";
		}
		
		str +="			<label for='"+prefix+"_portname'>Port Name </label><input type='text' id='"+prefix+"_portname' name='"+prefix+"_portname' /><br/>"+
		"			Port Rotation <select id='"+prefix+"_portrotation' name='"+prefix+"_portrotation'"+
        ">" +
		"											<option value='0'>0</option>"+
		"											<option value='90'>90</option>"+
		"											<option value='180'>180</option>"+
		"											<option value='270'>270</option>"+
		"										</select><br>"+
		"										<input type=checkbox name='"+prefix+"_flipport' id='"+prefix+"_flipport' value='1'>Flip port<br>"+
		"										<input type=checkbox name='"+prefix+"_centeroff' id='"+prefix+"_centeroff' value='1'>Center off<br>";
		if(document.f1.global_chipset.value == "TNC_B0" && prefix == "sdvo")
		{
			str +="				 <div id='"+prefix+"_displaytuning_div'><input type=checkbox name='"+prefix+"_displaytuning' id='"+prefix+"_displaytuning' value='1' checked='true'/>"+
			"<label for='"+prefix+"_displaytuning'>Enable display tuning</label><br></div>";
			str +="				 <div id='"+prefix+"_referencefrequency_div'><label for='"+prefix+"_referencefrequency'>Reference Frequency</label>"+ 
			"<input type=text name='"+prefix+"_referencefrequency' id='"+prefix+"_referencefrequency'/><br></div>";
		}
		
		str += "			<p>"+
		"<div> EDID Options "+
		"<select id='"+prefix+"_edid_option' name='"+prefix+"_edid_option' onchange='edidOptionsSelect(event)'><option Value='"+prefix+"_SimpleEDID'>Simple</option>"+
		"<option Value='"+prefix+"_AdvancedEDID'>Advanced</option></select>"+
	    "</div>"+
		"			<fieldset id='"+prefix+"_SimpleEDID' style='display:'><input type=checkbox name='"+prefix+"_nostandardtimings' id='"+prefix+"_nostandardtimings'><label for='"+prefix+"_nostandardtimings'>Disable standard timings</label><br>"+
		"			<input type=checkbox name='"+prefix+"_noEDID' id='"+prefix+"_noEDID' value='0'><label for='"+prefix+"_noEDID'>Disable EDID (timing from display)</label></fieldset>"+
		" <fieldset id='"+prefix+"_AdvancedEDID' style='display:none'>"+
		" <fieldset><legend>If EDID Device</legend>"+
		" <input type=checkbox name='"+prefix+"_edid_usestandardtimings' id='"+prefix+"_edid_usestandardtimings' value='1' checked='true'><label for='"+prefix+"_edid_usestandardtimings'>Use driver built-in standard timings</label><br>"+
		" <input type=checkbox name='"+prefix+"_edid_useedidblock' id='"+prefix+"_edid_useedidblock' value='2' checked='true'><label for='"+prefix+"_edid_useedidblock'>Use EDID Block</label><br>"+
		" <input type=checkbox name='"+prefix+"_edid_useuserdefined' id='"+prefix+"_edid_useuserdefined' value='4'><label for='"+prefix+"_edid_useuserdefined'>Use user-defined DTDs</label><br></fieldset>"+
		" <fieldset><legend>If Not EDID Device</legend>"+
		" <input type=checkbox name='"+prefix+"_notedid_usestandardtimings' id='"+prefix+"_notedid_usestandardtimings' value='1' checked='true'><label for='"+prefix+"_notedid_usestandardtimings'>Use driver built-in standard timings</label><br>"+
		" <input type=checkbox name='"+prefix+"_notedid_useuserdefined' id='"+prefix+"_notedid_useuserdefined' value='4'><label for='"+prefix+"_notedid_useuserdefined'>Use user-defined DTDs</label><br></fieldset>"+
		" </fieldset>"+
		"		</td>"+
		"		<td width='320px' valign='top'>"+

		"			<fieldset>"+
		"			<legend>Display Timing Descriptors (DTDs)</legend>"+
		"				<div id='"+prefix+"_dtds' style='width:300px;height:155px;overflow:auto'></div>"+
		"			</fieldset>"+

		"         </td>"+
		
		'		<td align="right" valign="top">'+
		'		<a href="help/web_ConfiguringLVDS-SDVOPorts.htm" target="_blank"><img src="images/questionmark_16w.gif"></a>'+

		'		</td>' +
		
		"         </tr>"+
		"         <tr>"+
		"	</table>";
			
return str;
}

function toggleDisplayTuning(event)
{
	if (!event) 
		event = window.event;
	
	var control = event.srcElement?event.srcElement:event.target;
	if(control.value == "TNC_B1") {
		document.getElementById('sdvo_displaytuning').checked = true;
		document.getElementById('sdvo_displaytuning_div').style.display = "none";
	} else	{
		document.getElementById('sdvo_displaytuning_div').style.display = "";
	}
}

function dtdControls(event)
{
	if (!event) 
		event = window.event;
	
	var control = event.srcElement?event.srcElement:event.target;
	
	processDTDControls(control.value);
	
}

function processDTDControls(control)
{
	var mainForm = document.f1;
	
	$("#DTD_parameters div").each(function(){
		var classVar = $(this).attr("class");
		if(classVar.indexOf("_parameters")>-1)
		{
			if(classVar.indexOf("emgd")<0 && classVar.indexOf(control)<0)
			{
				$(this).hide();
			}
			else if(classVar.indexOf(control)>-1)
			{
				$(this).show();
			}
		}
		
	});
	
	$("#DTD_parameters input").each(function(){
			mainForm[$(this).attr("id")].disabled = true;
		});
	$("#DTD_parameters select").each(function(){
			mainForm[$(this).attr("id")].disabled = true;
		});
	$("input."+control+"_parameters").each(function(){ mainForm[$(this).attr("id")].disabled = false;});		
	$("select."+control+"_parameters").each(function(){ mainForm[$(this).attr("id")].disabled = false;});
		
	if(control == "mode_lines")
	{
		$("#"+control).accordion("activate", 0);
		$("#edid_block").accordion("activate", false);
		$("#parameters").accordion("activate", false);
	}
	else if(control == "edid_block")
	{
		$("#"+control).accordion("activate", 0);
		$("#mode_lines").accordion("activate", false);
		$("#parameters").accordion("activate", false);
	}
	else
	{
		if(control=="simple")
			mainForm["vsync_polarity"].selectedIndex = 1;
		//to check if parameters accordion is open. if it is no need to activate.
		var active = $( "#parameters").accordion( "option", "active" );

		if(active == -1 || active+"" == "false")
			$("#parameters").accordion("activate", 0);
		$("#mode_lines").accordion("activate", false);
		$("#edid_block").accordion("activate", false);
	}
	dtdTranslation(control,previousSelection);
}

function addCustomAttribute(event)
{
	if (!event) 
		event = window.event;
	
	var control = event.srcElement?event.srcElement:event.target;
	
	var port = document.getElementById(control.id).getAttribute("port");
	var customAttrNo = $("#"+port+"_custom_composite tr").length - 1;
	addCustomAttributeByPort(port, customAttrNo);	
}

function addCustomAttributeByPort(port, num)
{		
	var name= port+"_attr_custom_"+num+"[]";
	var idBase = port+"_attr_custom_";
	
	var html = '<tr><td style="border: 1px solid #000">' + 
	'<input type="checkbox" name="'+idBase+'chkbox'+num+'" id="'+idBase+'chkbox_'+num+'" onClick="onAttributeClick(event);" value="'+name+'" />'+'custom'+num+"</td><td style=\"border: 1px solid #000\">"+
	"<input type=\"text\" id=\""+idBase+"txtID"+num+"\" disabled=\"disabled\" name=\""+name+"\" class=\"id\"/></td><td style=\"border: 1px solid #000\"> \
	<input type=\"text\" id=\""+idBase+"txtValue"+num+"\" disabled=\"disabled\" name=\""+name+"\" class=\"value\"/></td> \
	<td style=\"border: 1px solid #000\"><input type=\"text\" disabled=\"disabled\" id=\""+idBase+"txtName"+num+"\" name=\""+name+"\" class=\"name\"/></td></tr>";
	$("#"+port+"_custom_composite").append(html);
}

function enableCentering(event)
{
	if (!event) 
		event = window.event;
	
	var control = event.srcElement?event.srcElement:event.target;
	
	var controlArray = document.getElementsByName(control.value);
	
	for(i=0;i<controlArray.length;i++)
	{
		controlArray[i].disabled = control.checked;
	}
	
}

function timingDelays(event)
{
	if (!event) 
		event = window.event;
	
	var control = event.srcElement?event.srcElement:event.target;
	toggleTimingDelays(control);
}

function toggleTimingDelays(control)
{
	var disable  = control.value=="1"?"":"none";
	document.getElementById("timing_delays").style.display = disable;
}

function customDTDControlFunctions()
{
	var mainForm = document.f1;
	mainForm["dtd_name"].setSelection = function(selection){ if((selection+"").length > 0) mainForm["dtd_name"].value  = selection;};
	mainForm["dtd_type"].setSelection = function(selection){
				mainForm["dtd_type"].selectedIndex = selection-1;
	};
	
	$("input.emgd_parameters,input.vesa_parameters,input.hardware_parameters,input.mode_lines_parameters,input.simple_parameters, input.edid_block_parameters").each(function(){
			if(mainForm[$(this).attr("id")].type == "text")
			{
				mainForm[$(this).attr("id")].getSelection = function(){ return  mainForm[$(this).attr("id")].value;};
				mainForm[$(this).attr("id")].setSelection = function(selection){ if((selection+"").length > 0) mainForm[$(this).attr("id")].value  = selection;};
			}
			if(mainForm[$(this).attr("id")].type == "checkbox")
			{
				mainForm[$(this).attr("id")].getSelection =function() { return mainForm[$(this).attr("id")].checked?mainForm[$(this).attr("id")].value:0;};
				mainForm[$(this).attr("id")].setSelection =function(selection)
				{
				  mainForm[$(this).attr("id")].checked = (selection==0?false:true);
				};
			}
		});

		$("select.emgd_parameters").each(function(){
				mainForm[$(this).attr("id")].getSelection = function() { return mainForm[$(this).attr("id")].options[mainForm[$(this).attr("id")].selectedIndex].value;};
				mainForm[$(this).attr("id")].setSelection = function(selection) {
				  
				  for(var i=0;i<mainForm[$(this).attr("id")].options.length;i++)
				  {
					if(mainForm[$(this).attr("id")].options[i].value == selection)
					{
					  mainForm[$(this).attr("id")].selectedIndex = i;
					}
				  }					
				};
		});
}

function edidOptionsSelect(event)
{
	if (!event) 

		event = window.event;

	

	var control = event.srcElement?event.srcElement:event.target;
	
	document.getElementById(control.value).style.display = "";

	if(control.value.indexOf("SimpleEDID")>=0) {

		document.getElementById(control.options[1].value).style.display = "none";

	} else	{

		document.getElementById(control.options[0].value).style.display = "none";

	}
}

function resetTemplateArrays()
{
	mainTemplate = new Object();
	configTemplate = new Object();
	port = new Object();
	sharedTemplate = new Object();
}
