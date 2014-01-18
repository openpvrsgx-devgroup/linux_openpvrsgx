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

var displayMode;
var attrCounter = new Object();
attrCounter['lvds'] = 0;
attrCounter['sdvo'] = 0;

function configInput()
{
    configDisplay();    
    parseElements();
}

function configDisplay()
{
    var secondaryDisplay;
    var mainForm = document.f1;
    var primary_port_prefix = mainForm.global_cbo_primary_display.value+"";
    primary_port_prefix = primary_port_prefix.toLowerCase();

    if (primary_port_prefix == "lvds")
    {
      configTemplate["port_display"] = "4";
      secondaryDisplay = "2";
    } else
    {
        configTemplate["port_display"] = "2";
        secondaryDisplay = "4";
    }

    for (i=0;i<mainForm.global_radio.length;i++) {
            if (mainForm.global_radio[i].checked) {
                    displayMode = mainForm.global_radio[i].value+"";
            }
    }
    displayMode = displayMode.substring(displayMode.indexOf("/")+1);
    if (displayMode == "single.jpg")
    {
      //port[primary_port_prefix] = portTemplate;
   
      port[primary_port_prefix]["PORT_ID"] = configTemplate["port_display"];
    
      port[primary_port_prefix]['port_name'] = primary_port_prefix;
   
      configTemplate["port_display"] += "0000";
      configTemplate["config_name"] = "single";
      
      configTemplate["display_config_mode"] = "1";
   
    }
    else
    {
      //port['sdvo'] = portTemplate;
      port['sdvo']["PORT_ID"] = "2";
      port['sdvo']['port_name'] = "sdvo";
   
      //port['lvds'] = portTemplate;
      port['lvds']["PORT_ID"] = "4";
      port['lvds']['port_name'] = 'lvds';
   
   
       configTemplate["port_display"] += secondaryDisplay+"000";
   
       if (displayMode == "clone.jpg")
       {
	 configTemplate["display_config_mode"] = "2";
	 configTemplate["config_name"] = "clone";
       }
       else
       {
	 configTemplate["display_config_mode"] = "8";
	 if (displayMode == "extended.jpg")
	 {
	    configTemplate["config_name"] = "extended";
	    configTemplate["xinerama"] = "True";
	}
	else {
	    configTemplate["config_name"] = "DIH";
	    configTemplate["xinerama"] = "False";
	}
       }
    }
}

function isset(property)
{
    if(property != null && typeof(property) != 'undefined')
    {
       var type = property.type+"";
       if(type == "checkbox")
       {
           if(property.checked)
                    return true;
       }
       else if(type == "text")
       {
            if(property.value != '')
                return true;
       }
       else if(type == "select-one")
                return true;
        else
        {
            if(property.length>0)
            {
                for(i=0;i<property.length;i++)
                {
                    if(isset(property[i]))
                    return true;
                }
            }
           //alert(property.name+ " " +type+ " "+typeof(property));
        }
    }
  
    return false;
}

function edidProcessing(port_prefix)
{
/* EDID reference:
	001 use standard timing
	010 use edid block
	100 use DTD
*/

	var standardtiming = 1;
	var edidavail = 7;
	var edidnotavail = 5;
	var mainForm = document.f1;
	if(mainForm[port_prefix+"_edid_option"].value.indexOf("Simple")>=0)
	{
		if(isset(mainForm[port_prefix+"_nostandardtimings"]))  {
			edidavail = edidavail - standardtiming;
			edidnotavail = edidnotavail - standardtiming;
		}
		/*if (isset(mainForm[port_prefix+"_dtds"])) {
			edidavail+=4;
			edidnotavail+=4;
		}*/
		if(isset(mainForm[port_prefix+"_noEDID"]))	{
			edidavail = 0;
		}
	}
	else{
		edidavail = getValueIfChecked(mainForm[port_prefix+"_edid_usestandardtimings"])+getValueIfChecked(mainForm[port_prefix+"_edid_useedidblock"])+getValueIfChecked(mainForm[port_prefix+"_edid_useuserdefined"]);
		edidnotavail = getValueIfChecked(mainForm[port_prefix+"_notedid_usestandardtimings"])+getValueIfChecked(mainForm[port_prefix+"_notedid_useuserdefined"]);		
	}

	port[port_prefix]['disable_reading_edid'] = edidavail == 0?0:1;
	port[port_prefix]['IF_EDID_AVAILABLE'] = edidavail+"";
	port[port_prefix]['IF_EDID_NOT_AVAILABLE'] = edidnotavail+"";
}

function getValueIfChecked(control)
{
	return control.checked?parseInt(control.value):0;
}

function parseElements()
{
    var mainForm = document.f1;
    
    var primary_port_prefix = mainForm.global_cbo_primary_display.value+"";
    primary_port_prefix = primary_port_prefix.toLowerCase();
    
    if(displayMode == "single.jpg")
    {
		edidProcessing(primary_port_prefix);
    }
    else
    {
		edidProcessing('lvds');
		edidProcessing('sdvo');
    }

    var i;
	
	$.ajax({url: "xml/controls.xml", dataType: "xml", async: false,
		success: function(data)
		{
			$(data).find("port").each(function()
			{
				$(this).find("control").each(function(){
					var id = $(this).attr("id")+"";
				  
					var portName = id.split("_");
				  
					if(isset(mainForm[id+""]))
					{
						var value = mainForm[id+""].value 
						if ( value != '')
						{
							port[portName[0]+""][$(this).find("template").text()+""] =value;
						}
					}
				});
			});
		}
	});
 
    //TODO FIXME cleanup. Remove duplicate lvds/sdvo code
   for(i=0;i<mainForm.global_cbo_primary_display.length;i++)
   {
		var portName = mainForm.global_cbo_primary_display[i].value.toLowerCase();
		
		processSelectedDTD(portName);
		frameColorAttribute(portName);
		//port attributes
		attributeProcessing(portName);		
   }
    
    $.ajax({url: "xml/controls.xml",
		dataType: "xml",async: false,
		success: function(data)
		{
			$(data).find("config").each(function()
			{
				$(this).find("control").each(function(){
					var id = $(this).attr("id");
				  
					if(isset(mainForm[id+""]))
					{
						var value = mainForm[id+""].value;
						if ( value != '')
						{
							var templateStr = $(this).find("template").text()+"";
							if(templateStr != "port_display")
								configTemplate[templateStr] = value;
							if(templateStr == "overlay_gamma_correctionr" || templateStr == "overlay_gamma_correctiong" || templateStr == "overlay_gamma_correctionb")
								configTemplate[templateStr] = processOverlayGamma(value);
							else if(templateStr == "overlay_brightness_correction" || templateStr == "overlay_contrast_correction" || templateStr == "overlay_saturation_correction")
								configTemplate[templateStr] = computeBrightContrast(value, false);
						}
					}
				});
			});
		}
	});
	

	//flat panel attributes
	$.ajax({url: "xml/controls.xml", dataType: "xml", async: false,
		success: function(data)
		{
			$(data).find("attributes").each(function()
			{
				$(this).find("control").each(function(){
								  
					var id = $(this).attr("name")!=undefined?$(this).attr("name"):$(this).attr("id");
					var port_prefix = id.split("_")[0];
					
					if(typeof(port[port_prefix]['attr'][attrCounter[port_prefix]+""] == 'undefined'))
						port[port_prefix]['attr'][attrCounter[port_prefix]+""] = new Object();
												
					var template = $(this).find("template").text();
						
					if($(this).attr("id")!=undefined)
					{
						var control = document.getElementById($(this).attr("id"));

						if(isset(control))
						{
							$.ajax({url: "xml/controls_ced.xml", dataType: "xml", async: false,
									success: function(data){
										$(data).find('control[item_type_key$='+template+']').each(function(){ //select by attribute
											
											$(this).find("item").each(function(){
												port[port_prefix]['attr'][attrCounter[port_prefix]+""]['ATTR_ID'] = $(this).attr("item_value");
											});
										});
									}
							});
							
							port[port_prefix]['attr'][attrCounter[port_prefix]+""]['ATTR_VALUE'] = control.value;
							
							attrCounter[port_prefix]++;
						}
					}				
					else if($(this).attr("name")!=undefined)
					{
						var controlArray = document.getElementsByName($(this).attr("name"));
						
						for(i=0;i<controlArray.length;i++)
						{
							if(controlArray[i].checked && !controlArray[i].disabled)
							{
								$.ajax({url: "xml/controls_ced.xml", dataType: "xml", async: false,
										success: function(data){
											$(data).find('control[item_type_key$='+template+']').each(function(){ //select by attribute
												
												$(this).find("item").each(function(){
													port[port_prefix]['attr'][attrCounter[port_prefix]+""]['ATTR_ID'] = $(this).attr("item_value");
												});
											});
										}
								});
								
								port[port_prefix]['attr'][attrCounter[port_prefix]+""]['ATTR_VALUE'] = controlArray[i].value;
								attrCounter[port_prefix]++;
								break;
							}
						}
					
					}
				});
			});
		}
	});
}

function processSelectedDTD(port_prefix)
{
	var mainForm = document.f1;
	var dtdTable = mainForm[port_prefix+"_dtds"];
	
	if (isset(dtdTable))
    {
		count = 1;
		
		if(typeof(port[port_prefix]['dtd'] == 'undefined'))
			port[port_prefix]['dtd'] = new Object();
			
		for(i=0;i<dtdTable.length;i++)
		{
			if(dtdTable[i].checked)
			{
				countIndex = count+"";
			
				if(typeof(port[port_prefix]['dtd'][countIndex]) == 'undefined')
				{
					port[port_prefix]['dtd'][countIndex] = new Object();                    
				}
				
				port[port_prefix]['dtd'][countIndex]['DTD_ID'] = countIndex;
				
				if (dtdTable[i].value.indexOf("Custom ") >= 0) {		
					data = getXMLDoc(customDTDs[dtdTable[i].value]);
					parseDTDXML(data, port_prefix);
					count++;
				}
				else
				{
				$.ajax({url: "dtd/"+dtdTable[i].value+".dtd",
					dataType: "xml",async: false,
					success: function(data)
					{
						parseDTDXML(data, port_prefix);
						count++;
					}
				});
				}
			}
		}
    }

}

function computeDTDFlags(isNative, interlaceDisplay, vSynch, hSynch, bSynch)
{	
      var dtdFlag = 0;
	  
      if (isNative != null)
      {
         dtdFlag += Math.pow(2, 17);
      }
      dtdFlag += parseInt(bSynch);
      dtdFlag += parseInt(hSynch);
      dtdFlag += parseInt(vSynch);

      if (interlaceDisplay != null)
      {
         dtdFlag += parseInt(interlaceDisplay);
      }
      return "0x" + dtdFlag.toString(16);
}

function parseDTDXML(data, currentPort)
{
	var flagValues = new Array();
	
	$(data).find("selection").each(function()
	{
		itemType = $(this).attr("item_type_key")+"";
		itemValue = $(this).find("selection_entry").attr("entry_value");
		
		if(!(itemType == 'interlaced_display' || itemType == 'vsync_polarity' || itemType == 'hsync_polarity' || itemType == 'bsync_polarity'))
		{
			port[currentPort]['dtd'][countIndex][itemType] = itemValue;
			if(itemType == "dtd_name")
				flagValues[0] = itemValue;
		}
		else
		{
			if(itemValue != "undefined")
			{
				if(itemType == 'interlaced_display')
				{
					flagValues[1] = itemValue;
				}
				if(itemType == 'vsync_polarity')
				{
					flagValues[2] = itemValue;
				}
				if(itemType == 'hsync_polarity')
				{
					flagValues[3] = itemValue;
				}
				if(itemType == 'bsync_polarity')
				{
					flagValues[4] = itemValue;
				}
			}
		}
	});
	//TODO extract the right value of isNative
	var isNative = flagValues[0];
	port[currentPort]['dtd'][countIndex]['DTD_FLAGS'] = computeDTDFlags(isNative, flagValues[1], flagValues[2], flagValues[3], flagValues[4]);
}

function frameColorAttribute(port_prefix)
{
    var mainForm = document.f1;
	
	if(typeof(port[port_prefix]['attr'] == 'undefined'))
		port[port_prefix]['attr'] = new Object();
		
    var attributeArray = new Array();
    if(isset(mainForm[port_prefix+"_sliderValueGammaR"]) || isset(mainForm[port_prefix+"_sliderValueGammaG"]) || isset(mainForm[port_prefix+"_sliderValueGammaB"]))
    attributeArray = processGammaAttr(mainForm[port_prefix+"_sliderValueGammaR"].value,mainForm[port_prefix+"_sliderValueGammaG"].value,mainForm[port_prefix+"_sliderValueGammaB"].value);
    if(isset(mainForm[port_prefix+"_sliderValueBrightR"]) || isset(mainForm[port_prefix+"_sliderValueBrightG"])|| isset(mainForm[port_prefix+"_sliderValueBrightB"]))
    attributeArray = attributeArray.concat(processBrightContrastAttr(mainForm[port_prefix+"_sliderValueBrightR"].value,mainForm[port_prefix+"_sliderValueBrightG"].value,mainForm[port_prefix+"_sliderValueBrightB"].value, true));
    if(isset(mainForm[port_prefix+"_sliderValueContrastR"]) || isset(mainForm[port_prefix+"_sliderValueContrastG"])|| isset(mainForm[port_prefix+"_sliderValueContrastB"]))
    attributeArray = attributeArray.concat(processBrightContrastAttr(mainForm[port_prefix+"_sliderValueContrastR"].value,mainForm[port_prefix+"_sliderValueContrastG"].value,mainForm[port_prefix+"_sliderValueContrastB"].value, false));
    
    if(attributeArray != null)
    for(i=0;i<attributeArray.length;i=i+2, attrCounter[port_prefix]++)
    {
		if(typeof(port[port_prefix]['attr'][attrCounter[port_prefix]+""] == 'undefined'))
			port[port_prefix]['attr'][attrCounter[port_prefix]+""] = new Object();
		port[port_prefix]['attr'][attrCounter[port_prefix]+""]['ATTR_ID'] = attributeArray[i];
		port[port_prefix]['attr'][attrCounter[port_prefix]+""]['ATTR_VALUE'] = attributeArray[i+1];
    }
}

function attributeProcessing(port_prefix)
{
	var mainForm = document.f1;
	var compositeName = "#"+port_prefix+"_attributes_composite";
	$(compositeName).find("select").each(function(){
		if(!$(this).attr("disabled"))
		{
			if(typeof(port[port_prefix]['attr'][attrCounter[port_prefix]+""] == 'undefined'))
				port[port_prefix]['attr'][attrCounter[port_prefix]+""] = new Object();
			port[port_prefix]['attr'][attrCounter[port_prefix]+""]['ATTR_ID'] = $(this).attr("attr_id");
			port[port_prefix]['attr'][attrCounter[port_prefix]+""]['ATTR_VALUE'] = mainForm[$(this).attr("id")].options[mainForm[$(this).attr("id")].selectedIndex].value;
	
			attrCounter[port_prefix]++;
		}
	});
	
	$(compositeName).find("input").each(function(){
		if($(this).attr("type") != "checkbox" && !$(this).attr("disabled"))
		{
			if(typeof(port[port_prefix]['attr'][attrCounter[port_prefix]+""] == 'undefined'))
				port[port_prefix]['attr'][attrCounter[port_prefix]+""] = new Object();
			port[port_prefix]['attr'][attrCounter[port_prefix]+""]['ATTR_ID'] = $(this).attr("attr_id");
			port[port_prefix]['attr'][attrCounter[port_prefix]+""]['ATTR_VALUE'] = mainForm[$(this).attr("id")].value;
			attrCounter[port_prefix]++;
		}
	});
	
	compositeName = "#"+port_prefix+"_custom_composite";
	
	$(compositeName).find("input").each(function(){
		
		if($(this).attr("type") == "checkbox" && $(this).attr("checked"))
		{
			if(typeof(port[port_prefix]['attr'][attrCounter[port_prefix]+""] == 'undefined'))
				port[port_prefix]['attr'][attrCounter[port_prefix]+""] = new Object();
				
			var controlArray = document.getElementsByName($(this).attr("value"));
			
			port[port_prefix]['attr'][attrCounter[port_prefix]+""]['ATTR_ID'] = controlArray[0].value;
			port[port_prefix]['attr'][attrCounter[port_prefix]+""]['ATTR_VALUE'] = controlArray[1].value;
		
			attrCounter[port_prefix]++;
		}
	});
	
}
