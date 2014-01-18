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

function fillDTD_XML(prefix)
 {	
	var response = "";
	 $.ajax({url: "xml/dtds.xml", dataType: "xml", async: false,
		success: function(data)
		{
			$(data).find("DTD").each(function()
			{
				var idText = $(this).text().split(' ').join('_'); // use DTD name as part of the ID to ensure it is unique and easy to debug
				response = response + '<input type="checkbox" name="'+prefix+'_dtds" id="'+prefix+'_dtds_'+idText+'" value="'+$(this).text()+'">'+$(this).text()+'<br>';
			});
			document.getElementById(prefix+"_dtds").innerHTML=response;
		}
	 });
 } /* end function fillDTD_XML */


function fillSDVOCARD_XML()
 {
	 var response = "";
	 $.ajax({url: "xml/sdvo_cards.xml", dataType: "xml", async: false,
		success: function(data)
		{
			$(data).find("SCARD").each(function()
			{				
				var idText=$(this).attr("id");
				response = response + '<input type="checkbox" name="sdvo_checkbox" id="sdvo_checkbox_'+idText+'" value="'+$(this).text()+'" onclick="cardsCheck(event);" >'+$(this).text()+'<br>';
			});
			document.getElementById("sdvo1_cards_ckbox").innerHTML=response;
		}
	 });
 } /* end function fillSDVOCARD_XML */
 
 function fillLVDSAttributes(deviceName)
 {
	var count = 0;
    var previousDeviceName = "";
    if(deviceName == "Internal LVDS")
    {
      previousDeviceName = "Chrontel* CH7036";
      $('.flat_panel_settings').each(function()
							  {
								$(this).show();
                              });
    }
    else{
      previousDeviceName = "Internal LVDS";
      $('.flat_panel_settings').each(function()
							  {
								$(this).hide();
                              });
    }
    
   while($("#lvds_attributes_composite tr[id='" + previousDeviceName+count + "']").html() != null)
   {
	  $("#lvds_attributes_composite tr[id='" + previousDeviceName+count + "']").remove();
	  count++;
   }
	count = 0;
	$.ajax({url: "xml/deviceAttribute.xml", dataType: "xml", async: false,
			success: function(data)
			{
				$(data).find("Device").each(function()
				{
					if($(this).attr("device_name") == deviceName)
					{
						$(this).find("DeviceAttribute").each(function()
						{
							if(document.getElementById(deviceName+count) == null)
							{
								var id;
								deviceAttributeName = $(this).find("device_attribute_name").text();
								idNumber = $(this).find("id_number").text();
								defaultValue = $(this).find("default_value").text();
								valueControl = $(this).find("control").attr("control_type");
                                minimum_value = $(this).find("validation").attr("minimum_value");
                                maximum_value = $(this).find("validation").attr("maximum_value");
                                
								var checkId = "lvds_attr_chkbx_"+idNumber;
								if(valueControl == "default_edit_control")
								{
									id = "lvds_attr_txt_"+idNumber;
									inputControl = "<input type=\"text\" id=\""+id+"\" name=\"lvds_attr_"+idNumber+"\" attr_id=\""+idNumber+"\" value=\""+defaultValue+"\" \
                                    disabled=\"disabled\" label=\""+checkId+"_label\" minValue=\""+minimum_value+"\" maxValue=\""+maximum_value+"\" onBlur=\"validateUserInput(event);\" />";
								}
								else
								{
									id = "lvds_attr_select_"+idNumber;
									inputControl = "<select id=\""+id+"\" name=\"lvds_attr_"+idNumber+"\" attr_id=\""+idNumber+"\" disabled=\"disabled\">";
									$(this).find("items").each(function()
									{
										$(this).find("item").each(function()
										{
											if($(this).attr("item_value") == defaultValue)
												inputControl+="<option value=\""+$(this).attr("item_value")+"\" selected=\"selected\">"+$(this).attr("item_name")+"</option>";
											else
												inputControl+="<option value=\""+$(this).attr("item_value")+"\">"+$(this).attr("item_name")+"</option>";
										});
									});
									inputControl+="</select>";
								}
								str = "<tr id=\""+deviceName+count+"\"><td style=\"border: 1px solid #000\"> \
								<input type=\"checkbox\" name=\""+checkId+"\" id=\""+checkId+"\" value=\"lvds_attr_"+idNumber+"\" onclick=\"onAttributeClick(event);\"/><label for=\""+checkId+"\" id=\""+checkId+"_label\">"+deviceAttributeName+"</label></td> \
								<td style=\"border: 1px solid #000\">"+idNumber+
								"</td><td style=\"border: 1px solid #000\">"+inputControl+"</td><td style=\"border: 1px solid #000\">"
								+deviceName+"</td><td style=\"border: 1px solid #000\">"+defaultValue+"</td></tr>";
								
								$("#lvds_attributes_composite").append(str);
								
								count++;
							}
							
						});
						//return;
					}
				});
			}
		 });
 }
 
 function onAttributeClick(event)
 {
	var mainForm = document.f1;
	
	if (!event) 
		event = window.event;
	
	var control = event.srcElement?event.srcElement:event.target;
	
	/*var controlArray = control.value.split(",");
	for(controlEle in controlArray)
	{
		mainForm[controlArray[controlEle]].disabled = !control.checked;
	}	*/
	var controlArray = document.getElementsByName(control.value);
	
	for(i=0;i<controlArray.length;i++)
	{
		controlArray[i].disabled = !control.checked;
	}
 }
 
 
function setLVDSEncoder(event)
{
	if (!event) 
		event = window.event;
	
	var control = event.srcElement?event.srcElement:event.target;
	
	fillLVDSAttributes(control.value);
}
 
 function cardsCheck(event)
 {
	if (!event) 
		event = window.event;
	
	var control = event.srcElement?event.srcElement:event.target;
	
	populateAttributes(control);	
}

function populateAttributes(control)
{
	var deviceName = control.value;
	var count = 0;
	var mainForm = document.f1;
	
	if(control.checked)
	{
		$.ajax({url: "xml/deviceAttribute.xml", dataType: "xml", async: false,
			success: function(data)
			{
				$(data).find("Device").each(function()
				{
					if($(this).attr("device_name") == deviceName)
					{
						if($(this).attr("device_name") == "STM* IOH ConneXt")
						{
							mainForm["sdvo_i2cspeed"].value = "400";
						}
						
						$(this).find("DeviceAttribute").each(function()
						{
							if(document.getElementById(deviceName+count) == null)
							{
								deviceAttributeName = $(this).find("device_attribute_name").text();
								idNumber = $(this).find("id_number").text();
								defaultValue = $(this).find("default_value").text();
								valueControl = $(this).find("control").attr("control_type");
                                minimum_value = $(this).find("validation").attr("minimum_value");
                                maximum_value = $(this).find("validation").attr("maximum_value");
                                var checkId = "sdvo_attr_chkbx_"+idNumber;
								if(valueControl == "default_edit_control")
									inputControl = "<input type=\"text\" id=\"sdvo_attr_txt_"+idNumber+"\" name=\"sdvo_attr_"+idNumber+"\" attr_id=\""+idNumber+"\" value=\""+defaultValue+"\" disabled=\"disabled\" \
                                    label=\""+checkId+"_label\" minValue=\""+minimum_value+"\" maxValue=\""+maximum_value+"\" onBlur=\"validateUserInput(event);\"/>";
								else
								{
									inputControl = "<select id=\"sdvo_attr_select_"+idNumber+"\" name=\"sdvo_attr_"+idNumber+"\" attr_id=\""+idNumber+"\" disabled=\"disabled\">";
									$(this).find("items").each(function()
									{
										$(this).find("item").each(function()
										{
											if($(this).attr("item_value") == defaultValue)
												inputControl+="<option value=\""+$(this).attr("item_value")+"\" selected=\"selected\">"+$(this).attr("item_name")+"</option>";
											else
												inputControl+="<option value=\""+$(this).attr("item_value")+"\">"+$(this).attr("item_name")+"</option>";
										});
									});
									inputControl+="</select>";
								}
								str = "<tr id=\""+deviceName+count+"\"><td style=\"border: 1px solid #000\"><input type=\"checkbox\" name=\""+checkId+"\" id=\""+checkId+"\" value=\"sdvo_attr_"+idNumber+"\" onclick=\"onAttributeClick(event);\"/>\
                                <label for=\""+checkId+"\" id=\""+checkId+"_label\">"+deviceAttributeName+"</label></td> \
								<td style=\"border: 1px solid #000\"></td><td style=\"border: 1px solid #000\">"+idNumber+"</td><td style=\"border: 1px solid #000\">"+inputControl+"</td><td style=\"border: 1px solid #000\">"
								+deviceName+"</td><td style=\"border: 1px solid #000\">"+defaultValue+"</td></tr>";
															
								$("#sdvo_attributes_composite").append(str);
								
								count++;
							}							
						});
					}
				});
			}
		 });
		
		document.getElementById("sdvo_attributes_warning").style.display = "none";
	}
	else
	{
		while($("#sdvo_attributes_composite tr[id='" + deviceName+count + "']").html() != null)
		{
			$("#sdvo_attributes_composite tr[id='" + deviceName+count + "']").remove();
			count++;
		}
		
		if($("#sdvo_attributes_composite tr").length <= 1)
			document.getElementById("sdvo_attributes_warning").style.display = "";
	}
}
 
 
