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

//for polymorphism try getSelectionComboBox = function(){logic} and then in html assign the function to the getselection property
var previousSelection;
var customDTDParameters =  new Object();

var ASPECT_RATIO_4_3 = 4.0/3.0;
var ASPECT_RATIO_16_9 = 16.0/9.0;
var ASPECT_RATIO_16_10 = 16.0/10.0;
var ASPECT_RATIO_5_4 = 5.0/4.0;
var ASPECT_RATIO_15_9 = 15.0/9.0;

Number.prototype.toBinaryString = function() {
        
        if(!(this.toString().search(/^-?[0-9]+$/) == 0))
            return '';
        var num = this.valueOf();  
        var temp = Math.pow(2, 31); 
        if(num > ( temp * 2 - 1) || num < -temp ) 
            return '';
        
        var arr = new Array(32);         
        for(var i = 31; i >= 0; --i){ 
                arr[i] = (num & 1);
                num = num>>>1;
        }
        return arr.join('');  
}

function resetVariables()
{
    var mainForm = document.f1;
    $("input.emgd_parameters").each(function(){
            customDTDParameters[$(this).attr("id")] = "";
            
            if(mainForm[$(this).attr("id")].type == "checkbox")
            {
                customDTDParameters[$(this).attr("id")] = false;
            }
    });
        
    $("select.emgd_parameters").each(function(){
            customDTDParameters[$(this).attr("id")] = mainForm[$(this).attr("id")].options[0].text;
    });
}
    
function dtdTranslation(currentSelection, previousSelection)
{
    var mainForm = document.f1;
  //reset all the EMGD variables -> 0 or default
    resetVariables();
 
  //EMGD parameters
    if(previousSelection=="emgd")
    {
         //check for valid input then translate
        $("input.emgd_parameters").each(function(){
            
                customDTDParameters[$(this).attr("id")] = mainForm[$(this).attr("id")].getSelection();
        });
    
        $("select.emgd_parameters").each(function(){
               customDTDParameters[$(this).attr("id")] = mainForm[$(this).attr("id")].getSelection();
        });
    }
  
  //translate VESA -> EMGD
    else if(previousSelection=="vesa")
    {
       //check for valid input then translate. To do: replace the checks for empty string with appropriate validation function for each.

        if(validateIntString( mainForm["pixel_clock"].getSelection()))
        {
           customDTDParameters["pixel_clock"] = mainForm["pixel_clock"].getSelection();
        }
        //hSyncWidth
         if(validateIntString(mainForm["hztl_sync_pulse_offset"].getSelection()))
         {
           customDTDParameters["hztl_sync_pulse_offset"] = mainForm["hztl_sync_pulse_offset"].getSelection();
         }
         //hBlank
         if(validateIntString( mainForm["hztl_blanking"].getSelection()))
         {
            customDTDParameters["hztl_blanking"] = mainForm["hztl_blanking"].getSelection();
         }
          //vSyncWidth
         if(validateIntString(mainForm["vertical_sync_pulse_offset"].getSelection()))
         {
            customDTDParameters["vertical_sync_pulse_offset"] = mainForm["vertical_sync_pulse_offset"].getSelection();
         }
         //vBlank
         if(validateIntString(mainForm["vertical_blanking"].getSelection()))
         {
            customDTDParameters["vertical_blanking"] = parseInt(mainForm["vertical_blanking"].getSelection());
         }
        if(validateIntString(mainForm["vertical_blank_start"].getSelection()))
        {
           customDTDParameters["vertical_active"] =  (parseInt(mainForm["vertical_blank_start"].getSelection()) + 1);
        }
        if(validateIntString(mainForm["vertical_sync_start"].getSelection()))
        {
                if(validateIntString(customDTDParameters["vertical_active"]))
                {
                   customDTDParameters["vertical_sync_offset"] = (parseInt(mainForm["vertical_sync_start"].getSelection()) - parseInt(customDTDParameters["vertical_active"]) + 1);
                }
                else
                {
                    customDTDParameters["vertical_sync_offset"] = (parseInt(mainForm["vertical_sync_start"].getSelection()) + 1);
                }
        }
        if(validateIntString(mainForm["hztl_blank_start"].getSelection()))
        {
           customDTDParameters["hztl_active"] =  (parseInt(mainForm["hztl_blank_start"].getSelection()) + 1);                     
        }                  
        if(validateIntString(mainForm["hztl_sync_start"].getSelection()))
        {
              if(validateIntString(customDTDParameters["hztl_active"]))
               {
                  customDTDParameters["hztl_sync_offset"] = (parseInt(mainForm["hztl_sync_start"].getSelection()) - parseInt(customDTDParameters["hztl_active"]) + 1);
               }
               else
               {
                   customDTDParameters["hztl_sync_offset"] = (parseInt(mainForm["hztl_sync_start"].getSelection()) + 1);
               }
        }
      
        $("select.emgd_parameters").each(function(){
               customDTDParameters[$(this).attr("id")] = mainForm[$(this).attr("id")].getSelection();
        });
        
    }
  //translate HARDWARE -> EMGD
    else if(previousSelection=="hardware")
    {
       //check for valid input then translate
       if(validateIntString(mainForm["pixel_clock"].getSelection()))
       {
          customDTDParameters["pixel_clock"] = mainForm["pixel_clock"].getSelection();
       }
       if(validateIntString(mainForm["hztl_active"].getSelection()))
       {
          customDTDParameters["hztl_active"] = mainForm["hztl_active"].getSelection();
       }
       if(validateIntString(mainForm["hztl_sync_start"].getSelection()))
       {
          if(validateIntString(mainForm["hztl_active"].getSelection()))
          {
             customDTDParameters["hztl_sync_offset"] = (parseInt(mainForm["hztl_sync_start"].getSelection())-parseInt(mainForm["hztl_active"].getSelection()) + 1);
          }
          else
          {
             customDTDParameters["hztl_sync_offset"] = (parseInt(mainForm["hztl_sync_start"].getSelection()) + 1);
          }
       }
       if(validateIntString(mainForm["hztl_sync_end"].getSelection()))
       {
          if(customDTDParameters["hztl_active"] && customDTDParameters["hztl_sync_offset"])
          {
             customDTDParameters["hztl_sync_pulse_offset"] = (parseInt(mainForm["hztl_sync_end"].getSelection()) - 
                   parseInt(customDTDParameters["hztl_active"]) - parseInt(customDTDParameters["hztl_sync_offset"]) + 1);
          }
          else if(validateIntString(customDTDParameters["hztl_sync_offset"]))
          {
             customDTDParameters["hztl_sync_pulse_offset"] = (parseInt(mainForm["hztl_sync_end"].getSelection()) - 
                   parseInt(customDTDParameters["hztl_sync_offset"]) + 1);
          }
          else if(validateIntString(customDTDParameters["hztl_sync_offset"]))
          {
             customDTDParameters["hztl_sync_pulse_offset"] = (parseInt(mainForm["hztl_sync_end"].getSelection()) - 
                   parseInt(customDTDParameters["hztl_sync_offset"]) + 1);
          }
          else
          {
             customDTDParameters["hztl_sync_offset"] = (parseInt(mainForm["hztl_sync_start"]) + 1);
          }
       }
       
       if(validateIntString(mainForm["hztl_blank_end"].getSelection()))
       {
          customDTDParameters["hztl_blanking"] = (parseInt(mainForm["hztl_blank_end"].getSelection()) - 
               parseInt(customDTDParameters["hztl_active"]) + 1);
       }
       
       if(validateIntString(mainForm["vertical_active"].getSelection()))
       {
          customDTDParameters["vertical_active"] = mainForm["vertical_active"].getSelection();
       }
       
       if(validateIntString(mainForm["vertical_sync_start"].getSelection()))
       {
          if(customDTDParameters["vertical_active"])
          {
             customDTDParameters["vertical_sync_offset"] =(parseInt(mainForm["vertical_sync_start"].getSelection())- 
                   parseInt(customDTDParameters["vertical_active"]) + 1);
          }
          else
          {
            customDTDParameters["vertical_sync_offset"] = (parseInt(mainForm["vertical_sync_start"].getSelection()) + 1);
          }
       }
       
       if( validateIntString(mainForm["vertical_sync_end"].getSelection()))
       {
          if((customDTDParameters["vertical_active"] != "") && (customDTDParameters["vertical_sync_offset"] != ""))
          {
             customDTDParameters["vertical_sync_pulse_offset"] = (parseInt(mainForm["vertical_sync_end"].getSelection()) - 
                   parseInt(customDTDParameters["vertical_active"]) - parseInt(customDTDParameters["vertical_sync_offset"]) + 1);
          }
          else if(validateIntString(customDTDParameters["vertical_active"]))
          {
             customDTDParameters["vertical_sync_pulse_offset"] = (parseInt(mainForm["vertical_sync_end"].getSelection()) - 
                  parseInt(customDTDParameters["vertical_active"]) + 1);
          }
          else if(validateIntString(customDTDParameters["vertical_sync_offset"]))
          {
             customDTDParameters["vertical_sync_pulse_offset"] = (parseInt(mainForm["vertical_sync_end"].getSelection()) - 
                  parseInt(customDTDParameters["vertical_sync_offset"]) + 1);
          }
          else
          {
             customDTDParameters["vertical_sync_pulse_offset"] = (parseInt(mainForm["vertical_sync_end"].getSelection()) + 1);
          }
       }
  
       if(validateIntString(mainForm["vertical_blank_end"].getSelection()))
       {
          if(customDTDParameters["vertical_active"] != "")
          {
             customDTDParameters["vertical_blanking"] = (parseInt(mainForm["vertical_blank_end"].getSelection()) - 
                   parseInt(customDTDParameters["vertical_active"]) + 1);
          }
          else
          {
             customDTDParameters["vertical_blanking"] = (parseInt(mainForm["vertical_blank_end"].getSelection()) + 1);
          }
       }

       //dtd flag
       customDTDParameters["interlaced_display"] = mainForm["interlaced_display"].getSelection();
       customDTDParameters["vsync_polarity"] = mainForm["vsync_polarity"].getSelection();
       customDTDParameters["hsync_polarity"] = mainForm["hsync_polarity"].getSelection();
      
    }
    
    // translate Mode Line -> EMGD
    else if(previousSelection=="mode_lines")
    {
       //check for valid input then translate
       if(validateIntString(mainForm["mode_line_A"].getSelection()))
       {
          customDTDParameters["hztl_active"] = mainForm["mode_line_A"].getSelection();
       }
       if(validateIntString(mainForm["mode_line_B"].getSelection()))
       {
          if(validateIntString(customDTDParameters["hztl_active"]))
          {
             customDTDParameters["hztl_sync_offset"] =(parseInt(mainForm["mode_line_B"].getSelection()) - 
                   parseInt(customDTDParameters["hztl_active"]));
          }
          else
          {
             customDTDParameters["hztl_sync_offset"] = mainForm["mode_line_B"].getSelection();
          }
       }
       if(validateIntString(mainForm["mode_line_C"].getSelection()))
       {
          if(validateIntString(customDTDParameters["hztl_active"]))
          {
             customDTDParameters["hztl_sync_pulse_offset"] = (parseInt(mainForm["mode_line_C"].getSelection()) - 
                   parseInt(customDTDParameters["hztl_active"]));
          }
          else
          {
             customDTDParameters["hztl_sync_pulse_offset"] = mainForm["mode_line_C"].getSelection();
          }
       }
       if(validateIntString(mainForm["mode_line_D"].getSelection()))
       {
          if(validateIntString(customDTDParameters["hztl_active"]))
          {
             customDTDParameters["hztl_blanking"] = (parseInt(mainForm["mode_line_D"].getSelection()) - 
                   parseInt(customDTDParameters["hztl_active"]));
          }
          else
          {
             customDTDParameters["hztl_blanking"] = mainForm["mode_line_D"].getSelection();
          }
       }
       if(validateFloatString(mainForm["mode_line_I"].getSelection()))
       {
          customDTDParameters["pixel_clock"] = round(parseFloat( mainForm["mode_line_I"].getSelection()) * 1000);
       }
       if(validateIntString(mainForm["mode_line_E"].getSelection()))
       {
          customDTDParameters["vertical_active"] = mainForm["mode_line_E"].getSelection();
       }
       if(validateIntString(mainForm["mode_line_F"].getSelection()))
       {
          if(validateIntString(customDTDParameters["vertical_active"]))
          {
             customDTDParameters["vertical_sync_offset"] = (parseInt(mainForm["mode_line_F"].getSelection()) - 
                   parseInt(customDTDParameters["vertical_active"]));
          }
          else
          {
             customDTDParameters["vertical_sync_offset"] = mainForm["mode_line_F"].getSelection();
          }
       }
       if(validateIntString(mainForm["mode_line_G"].getSelection()))
       {
          if(validateIntString(customDTDParameters["vertical_active"]))
          {
             customDTDParameters["vertical_sync_pulse_offset"] = (parseInt(mainForm["mode_line_G"].getSelection()) - 
                   parseInt(customDTDParameters["vertical_active"]));
          }
          else
          {
             customDTDParameters["vertical_sync_pulse_offset"] = mainForm["mode_line_G"].getSelection();
          }
       }
       if(validateIntString(mainForm["mode_line_H"].getSelection()))
       {
          if(validateIntString(customDTDParameters["vertical_active"]))
          {
             customDTDParameters["vertical_blanking"] = (parseInt(mainForm["mode_line_H"].getSelection()) - 
                   parseInt(customDTDParameters["vertical_active"]));
          }
          else
          {
             customDTDParameters["vertical_blanking"] = mainForm["mode_line_H"].getSelection();
          }
       }
       //dtd flags
       customDTDParameters["interlaced_display"] = mainForm["interlaced_display"].getSelection();
       customDTDParameters["vsync_polarity"] = mainForm["vsync_polarity"].getSelection();
       customDTDParameters["hsync_polarity"] = mainForm["hsync_polarity"].getSelection();
    }
    
    //EDID -> EMGD parameters
    else if(previousSelection=="edid_block")
    {
       //check for valid input then translate
       //customDTDParameters["pixel_clock"]
       var ZERO = "0";
       var SET = "1";
       //if(IStatus.OK == (mainForm["edid_block_1"].validateControl().getSeverity()) && IStatus.OK == 
        //  (mainForm["edid_block_2"].validateControl().getSeverity()))
      
        if(validateByteHexValue(mainForm["edid_block_1"].getSelection()) && validateByteHexValue(mainForm["edid_block_2"].getSelection()))
       {
          var sByte1 = mainForm["edid_block_1"].getSelection()+"";
          if(sByte1.length==1)
          {
             sByte1 = ZERO+(sByte1);
          }
          customDTDParameters["pixel_clock"] = (parseInt((mainForm["edid_block_2"].getSelection())+(sByte1), 16)*10);
       }
       //customDTDParameters["hztl_active"] and customDTDParameters["hztl_blanking"]
       //if(IStatus.OK == (mainForm["edid_block_3"].validateControl().getSeverity()) && IStatus.OK == (mainForm["edid_block_4"].
         //  validateControl().getSeverity()) && IStatus.OK == (mainForm["edid_block_5"].validateControl().getSeverity()))
         if(validateByteHexValue(mainForm["edid_block_3"].getSelection()) && validateByteHexValue(mainForm["edid_block_4"].getSelection()) && validateByteHexValue(mainForm["edid_block_5"].getSelection())) 
       {
          var sByte3 = mainForm["edid_block_3"].getSelection()+"";
          if(sByte3.length==1)
          {
             sByte3 = ZERO+sByte3;
          }
          var sByte4 = mainForm["edid_block_4"].getSelection()+"";
          if(sByte4.length==1)
          {
             sByte4 = ZERO+sByte4;
          }
          var sByte5 = mainForm["edid_block_5"].getSelection()+"";
          if(sByte5.length==1)
          {
             sByte5 = ZERO+sByte5;
          }
          customDTDParameters["hztl_active"] = (parseInt(sByte5.substring(0,1)+(sByte3), 16));
          customDTDParameters["hztl_blanking"] = (parseInt(sByte5.substring(1, sByte5.length)+(sByte4), 16));
       }
      // else if(IStatus.OK == (mainForm["edid_block_3"].validateControl().getSeverity()) && 
       //      IStatus.OK == (mainForm["edid_block_5"].validateControl().getSeverity()))
         if(validateByteHexValue(mainForm["edid_block_3"].getSelection()) && validateByteHexValue(mainForm["edid_block_5"].getSelection())) 
       {
          var sByte3 = mainForm["edid_block_3"].getSelection()+"";
          if(sByte3.length==1)
          {
             sByte3 = ZERO+sByte3;
          }
          var sByte5 = mainForm["edid_block_5"].getSelection()+"";
          if(sByte5.length==1)
          {
             sByte5 = ZERO+sByte5;
          }
          customDTDParameters["hztl_active"] = (parseInt(sByte5.substring(0,1)+(sByte3), 16));
       }
       //else if(IStatus.OK == (mainForm["edid_block_4"].validateControl().getSeverity()) && IStatus.OK == (mainForm["edid_block_5"].validateControl()
        //     .getSeverity()))
        if(validateByteHexValue(mainForm["edid_block_4"].getSelection()) && validateByteHexValue(mainForm["edid_block_5"].getSelection())) 
       {
          var sByte4 = mainForm["edid_block_4"].getSelection()+"";
          if(sByte4.length==1)
          {
             sByte4 = ZERO+sByte4;
          }
          var sByte5 = mainForm["edid_block_5"].getSelection()+"";
          if(sByte5.length==1)
          {
             sByte5 = ZERO+sByte5;
          }
          customDTDParameters["hztl_blanking"] = (parseInt(sByte5.substring(1,sByte5.length)+(sByte4), 16));
       }
       //customDTDParameters["hztl_sync_offset"]
       //if((IStatus.OK == (mainForm["edid_block_9"].validateControl().getSeverity())) )
        if(validateByteHexValue(mainForm["edid_block_9"].getSelection())) 
       {
          customDTDParameters["hztl_sync_offset"] = (parseInt(mainForm["edid_block_9"].getSelection(), 16));
       }
       //customDTDParameters["hztl_sync_pulse_offset"]
       //if(IStatus.OK == (mainForm["edid_block_10"].validateControl().getSeverity()))
        if(validateByteHexValue(mainForm["edid_block_10"].getSelection())) 
       {
          customDTDParameters["hztl_sync_pulse_offset"] = (parseInt( mainForm["edid_block_10"].getSelection(), 16));
       }
       //customDTDParameters["vertical_active"] and customDTDParameters["vertical_blank_end"]
       //if(IStatus.OK == (mainForm["edid_block_6"].validateControl().getSeverity()) && IStatus.OK == (mainForm["edid_block_7"].
       //    validateControl().getSeverity()) && IStatus.OK == (mainForm["edid_block_8"].validateControl().getSeverity()))
       if(validateByteHexValue(mainForm["edid_block_6"].getSelection()) && validateByteHexValue(mainForm["edid_block_7"].getSelection()) && validateByteHexValue(mainForm["edid_block_8"].getSelection())) 
       {
          var sByte6 = mainForm["edid_block_6"].getSelection()+"";
          if(sByte6.length==1)
          {
             sByte6 = ZERO+sByte6;
          }
          var sByte7 = mainForm["edid_block_7"].getSelection()+"";
          if(sByte7.length==1)
          {
             sByte7 = ZERO+(sByte7);
          }
          var sByte8 = mainForm["edid_block_8"].getSelection()+"";
          if(sByte8.length==1)
          {
             sByte8 = ZERO+(sByte8);
          }
          customDTDParameters["vertical_active"] = (parseInt(sByte8.substring(0,1)+(sByte6), 16));
          customDTDParameters["vertical_blank_end"] = (parseInt(sByte8.substring(1,sByte8.length)+(sByte7), 16));
       }
      // else if(IStatus.OK == (mainForm["edid_block_6"].validateControl().getSeverity()) && IStatus.OK == (mainForm["edid_block_8"].validateControl()
       //      .getSeverity()))
       if(validateByteHexValue(mainForm["edid_block_6"].getSelection()) && validateByteHexValue(mainForm["edid_block_8"].getSelection())) 
       {
          var sByte6 = mainForm["edid_block_6"].getSelection()+"";
          if(sByte6.length==1)
          {
             sByte6 = ZERO+(sByte6);
          }
          var sByte8 = mainForm["edid_block_8"].getSelection()+"";
          if(sByte8.length==1)
          {
             sByte8 = ZERO+(sByte8);
          }
          customDTDParameters["vertical_active"] = parseInt(sByte8.substring(0,1)+(sByte6), 16);
       }
       //else if(IStatus.OK == (mainForm["edid_block_7"].validateControl().getSeverity()) && IStatus.OK == (mainForm["edid_block_8"].validateControl()
        //     .getSeverity()))
        if(validateByteHexValue(mainForm["edid_block_7"].getSelection()) && validateByteHexValue(mainForm["edid_block_8"].getSelection())) 
       {
          var sByte7 = mainForm["edid_block_7"].getSelection()+"";
          if(sByte7.length==1)
          {
             sByte7 = ZERO+(sByte7);
          }
          var sByte8 = mainForm["edid_block_8"].getSelection()+"";
          if(sByte8.length==1)
          {
             sByte8 = ZERO+(sByte8);
          }
          customDTDParameters["vertical_blanking"] = (parseInt(sByte8.substring(1,sByte8.length)+(sByte7), 16));
       }
       //customDTDParameters["vertical_sync_offset"]
       //if(IStatus.OK == (mainForm["edid_block_11"].validateControl().getSeverity()))
       if(validateByteHexValue(mainForm["edid_block_11"].getSelection())) 
       {
          var sByte11 = mainForm["edid_block_11"].getSelection()+"";
          if(sByte11.length==1)
          {
             sByte11 = ZERO+(sByte11);
          }
          customDTDParameters["vertical_sync_offset"] = (parseInt(sByte11.substring(0,1), 16));
          customDTDParameters["vertical_sync_pulse_offset"] = (parseInt(sByte11.substring(1, sByte11.length), 16));
       }
       //dtd flags
       //if(IStatus.OK == (mainForm["edid_block_18"].validateControl().getSeverity()))
       if(validateByteHexValue(mainForm["edid_block_18"].getSelection())) 
       {
          var sByte18 = (parseInt(mainForm["edid_block_18"].getSelection(), 16)).toBinaryString();
          sByte18 = sByte18.substring(sByte18.length - 8,sByte18.length);
          while(sByte18.length<8)
          {
             sByte18 = ZERO+(sByte18);
          }
          if(sByte18.substring(0, 1)==(SET))
          {
             customDTDParameters["interlaced_display"] = 2147483648;
          }
          if(sByte18.substring(3, 4)==(SET) && sByte18.substring(4, 5)==(SET))
          {
             if(sByte18.substring(5, 6)==(SET))
             {
                customDTDParameters["vsync_polarity"] = 134217728;
             }
             if(sByte18.substring(6, 7)==(SET))
             {
                customDTDParameters["hsync_polarity"] = 67108864;
             }
          }
       }
    }
    
    //Simple -> EMGD parameters
    else if(previousSelection=="simple")
    {
       //check for valid input then translate
       //customDTDParameters["hztl_active"]
       if(validateIntString(mainForm["hztl_active"].getSelection()))
       {
          customDTDParameters["hztl_active"] = (parseInt(mainForm["hztl_active"].getSelection()));
       }
       //customDTDParameters["vertical_active"]
       if(validateIntString(mainForm["vertical_active"].getSelection()))
       {
          customDTDParameters["vertical_active"] = (parseInt(mainForm["vertical_active"].getSelection()));
       }
       
       if(validateIntString(customDTDParameters["hztl_active"]) && validateIntString(customDTDParameters["vertical_active"]))
       {
          //customDTDParameters["vertical_sync_offset"]
          customDTDParameters["vertical_sync_offset"] = 3;
          
          //customDTDParameters["vertical_sync_pulse_offset"]
          var aspectRatio = (parseFloat(mainForm["hztl_active"].getSelection())/
                parseFloat(mainForm["vertical_active"].getSelection()));
          if(aspectRatio == ASPECT_RATIO_4_3)
          {
             customDTDParameters["vertical_sync_pulse_offset"] = 4;
          }
          else if(aspectRatio == ASPECT_RATIO_16_9)
          {
             customDTDParameters["vertical_sync_pulse_offset"] = 5;
          }
          else if(aspectRatio == ASPECT_RATIO_16_10)
          {
             customDTDParameters["vertical_sync_pulse_offset"] = 6;
          }
          else if(aspectRatio == ASPECT_RATIO_5_4 || aspectRatio == ASPECT_RATIO_15_9)
          {
             customDTDParameters["vertical_sync_pulse_offset"] =7;
          }
          else
          {
             customDTDParameters["vertical_sync_pulse_offset"] = 10;
             //warning on page....not CVT standard
          }
  
          //customDTDParameters["vertical_blank_end"]
          // if interlace is required INT_RQD is set to true
          var INT_RQD = false;
          var INTERLACE = 0;
          var MIN_VSYNC_BP = 550;
          var MIN_V_PORCH_RND = 3;
          var MIN_V_BPORCH = 6;
          if(mainForm["interlaced_display"].getSelection())
          {
             INT_RQD = true;
             INTERLACE = 0.5;
          }
          if(validateIntString(mainForm["refresh"].getSelection()))
          {
             var V_FIELD_RATE_RQD = parseInt( mainForm["refresh"].getSelection());
             if (INT_RQD)
             {
                V_FIELD_RATE_RQD = V_FIELD_RATE_RQD * 2;
             }
             var V_LINES_RND = parseInt(customDTDParameters["vertical_active"]);
             if (INT_RQD)
             {
                V_LINES_RND = V_LINES_RND / 2;
             }
             var H_PERIOD_EST = ((1 / V_FIELD_RATE_RQD) - MIN_VSYNC_BP / 1000000.0) / 
                                  (V_LINES_RND + MIN_V_PORCH_RND + INTERLACE) * 1000000.0;
             var V_SYNC_BP = round(MIN_VSYNC_BP / H_PERIOD_EST) + 1;
             if (V_SYNC_BP < (parseInt(customDTDParameters["vertical_sync_pulse_offset"]) + MIN_V_BPORCH))
             {
                V_SYNC_BP = parseInt(customDTDParameters["vertical_sync_pulse_offset"]) + MIN_V_BPORCH;
             }
             customDTDParameters["vertical_blanking"] = round(V_SYNC_BP + MIN_V_PORCH_RND);
  
             // customDTDParameters["hztl_blanking"]
             var C_PRIME = 30;
             var M_PRIME = 300;
             var CELL_GRAN_RND = 8;
             var IDEAL_DUTY_CYCLE = C_PRIME - (M_PRIME * H_PERIOD_EST / 1000);
             if (IDEAL_DUTY_CYCLE < 20.0)
             {
                customDTDParameters["hztl_blanking"] = (round(((customDTDParameters["hztl_active"]) * 20.0 / (100.0 - 20.0) / 
                      (2.0 * CELL_GRAN_RND))) * round(2.0 * CELL_GRAN_RND));
             }
             else
             {
                customDTDParameters["hztl_blanking"] = (round(((customDTDParameters["hztl_active"]) * IDEAL_DUTY_CYCLE / (100.0 - IDEAL_DUTY_CYCLE) / (2.0 * CELL_GRAN_RND)))
                      * round(2.0 * CELL_GRAN_RND));
             }
  
             // customDTDParameters["hztl_sync_pulse_offset"]
             var H_SYN_PER = 8;
             var TOTAL_PIXELS = parseFloat(customDTDParameters["hztl_active"]) + parseFloat(customDTDParameters["hztl_blanking"]);
             customDTDParameters["hztl_sync_pulse_offset"] = (round(H_SYN_PER / 100.0 * TOTAL_PIXELS / CELL_GRAN_RND)
                * round(CELL_GRAN_RND));
  
             // customDTDParameters["hztl_sync_offset"]
             customDTDParameters["hztl_sync_offset"] = (round(parseFloat(customDTDParameters["hztl_blanking"]) / 2.0 - 
                   parseFloat(customDTDParameters["hztl_sync_pulse_offset"])));
  
             // customDTDParameters["pixel_clock"]
             customDTDParameters["pixel_clock"] = Math.round((TOTAL_PIXELS)/H_PERIOD_EST*10.0)*100;
  
             // dtd flags
             customDTDParameters["interlaced_display"] = mainForm["interlaced_display"].getSelection();
             customDTDParameters["vsync_polarity"] = 134217728; //active high
             customDTDParameters["hsync_polarity"] = 0;
          }
       }
    }
   
    populateDTD(currentSelection);
    setPreviousSelection(currentSelection);
}

function populateDTD(currentSelection)
{
    var mainForm = document.f1;
      //populate dtd type
      if(currentSelection=="emgd")
      {
        $("input.emgd_parameters").each(function(){
                mainForm[$(this).attr("id")].setSelection(customDTDParameters[$(this).attr("id")]);
        });
    
        $("select.emgd_parameters").each(function(){
                mainForm[$(this).attr("id")].setSelection(customDTDParameters[$(this).attr("id")]);
        });
      }
      else if(currentSelection=="vesa")
      {
         mainForm["pixel_clock"].setSelection(customDTDParameters["pixel_clock"]);
         mainForm["hztl_blank_start"].setSelection(subtract1(customDTDParameters["hztl_active"]));
         mainForm["hztl_sync_start"].setSelection((add(parseInt(customDTDParameters["hztl_active"]), true, parseInt(customDTDParameters["hztl_sync_offset"]), null)));
         mainForm["hztl_sync_pulse_offset"].setSelection(customDTDParameters["hztl_sync_pulse_offset"]);
         mainForm["hztl_blanking"].setSelection(customDTDParameters["hztl_blanking"]);
         mainForm["vertical_blank_start"].setSelection(subtract1(customDTDParameters["vertical_active"]));
         mainForm["vertical_sync_start"].setSelection(add(parseInt(customDTDParameters["vertical_active"]), true, parseInt(customDTDParameters["vertical_sync_offset"]), null));
         mainForm["vertical_sync_pulse_offset"].setSelection(customDTDParameters["vertical_sync_pulse_offset"]);
         mainForm["vertical_blanking"].setSelection(customDTDParameters["vertical_blanking"]);
         mainForm["interlaced_display"].setSelection(customDTDParameters["interlaced_display"]);
         mainForm["vsync_polarity"].setSelection(customDTDParameters["vsync_polarity"]);
         mainForm["hsync_polarity"].setSelection(customDTDParameters["hsync_polarity"]);
      }
      else if(currentSelection=="hardware")
      {
         mainForm["pixel_clock"].setSelection(customDTDParameters["pixel_clock"]);
         mainForm["hztl_active"].setSelection(customDTDParameters["hztl_active"]);
         mainForm["hztl_blank_end"].setSelection(add(customDTDParameters["hztl_active"], true, customDTDParameters["hztl_blanking"], null));
         mainForm["hztl_sync_start"].setSelection(add(customDTDParameters["hztl_active"], true, customDTDParameters["hztl_sync_offset"], null));
         mainForm["hztl_sync_end"].setSelection(add(customDTDParameters["hztl_active"], true, customDTDParameters["hztl_sync_offset"], customDTDParameters["hztl_sync_pulse_offset"]));
         mainForm["vertical_active"].setSelection(customDTDParameters["vertical_active"]);
         mainForm["vertical_blank_end"].setSelection(add(customDTDParameters["vertical_active"], true, customDTDParameters["vertical_blanking"], null));
         mainForm["vertical_sync_start"].setSelection(add(customDTDParameters["vertical_active"], true, customDTDParameters["vertical_sync_offset"], null));
         mainForm["vertical_sync_end"].setSelection(add(customDTDParameters["vertical_active"], true, customDTDParameters["vertical_sync_offset"], customDTDParameters["vertical_sync_pulse_offset"]));
         mainForm["interlaced_display"].setSelection(customDTDParameters["interlaced_display"]);
         mainForm["vsync_polarity"].setSelection(customDTDParameters["vsync_polarity"]);
         mainForm["hsync_polarity"].setSelection(customDTDParameters["hsync_polarity"]);
      }
      else if(currentSelection=="mode_lines")
      {
         if(!customDTDParameters["pixel_clock"]=="")
         {
            mainForm["mode_line_I"].setSelection((parseInt(customDTDParameters["pixel_clock"])/1000));
         }
         else
         {
             mainForm["mode_line_I"].setSelection(customDTDParameters["pixel_clock"]);
         }
         mainForm["mode_line_A"].setSelection(customDTDParameters["hztl_active"]);
         mainForm["mode_line_B"].setSelection(add(customDTDParameters["hztl_active"], false, customDTDParameters["hztl_sync_offset"], null));
         mainForm["mode_line_C"].setSelection(add(customDTDParameters["hztl_active"], false, customDTDParameters["hztl_sync_pulse_offset"], null));
         mainForm["mode_line_D"].setSelection(add(customDTDParameters["hztl_active"], false, customDTDParameters["hztl_blanking"], null));
         mainForm["mode_line_E"].setSelection(customDTDParameters["vertical_active"]);
         mainForm["mode_line_F"].setSelection(add(customDTDParameters["vertical_active"], false, customDTDParameters["vertical_sync_offset"], null));
         mainForm["mode_line_G"].setSelection(add(customDTDParameters["vertical_active"], false, customDTDParameters["vertical_sync_pulse_offset"], null));
         mainForm["mode_line_H"].setSelection(add(customDTDParameters["vertical_active"], false, customDTDParameters["vertical_blanking"], null));
         mainForm["interlaced_display"].setSelection(customDTDParameters["interlaced_display"]);
         mainForm["vsync_polarity"].setSelection(customDTDParameters["vsync_polarity"]);
         mainForm["hsync_polarity"].setSelection(customDTDParameters["hsync_polarity"]);
      }
      else if(currentSelection=="edid_block")
      {
         edidBlockComputations();
      }
      else if(currentSelection=="simple")
      {
         if(!customDTDParameters["pixel_clock"]=="")
         {
            customDTDParameters["pixel_clock"] = (round(parseInt(customDTDParameters["pixel_clock"]) * 1000 / 
                  ((parseInt(customDTDParameters["hztl_active"]) -1 + parseInt(customDTDParameters["hztl_blanking"])) * (parseInt(customDTDParameters["vertical_active"]) - 1 + 
                        parseInt(customDTDParameters["vertical_blanking"])))))+"";
         }
         mainForm["refresh"].setSelection(customDTDParameters["pixel_clock"]);
         mainForm["hztl_active"].setSelection(customDTDParameters["hztl_active"]);
         mainForm["vertical_active"].setSelection(customDTDParameters["vertical_active"]);
         mainForm["interlaced_display"].setSelection(customDTDParameters["interlaced_display"]);
         mainForm["vsync_polarity"].setSelection(customDTDParameters["vsync_polarity"]);
         mainForm["hsync_polarity"].setSelection(customDTDParameters["hsync_polarity"]);
      }
}

function edidBlockComputations()
{
    var mainForm = document.f1;
    var ZERO = "0";
   if(customDTDParameters["pixel_clock"].length>0)
   {
      var pClock = parseInt(parseInt(customDTDParameters["pixel_clock"])/10).toString(16);
      while(pClock.length<4)
      {
         pClock = ZERO+pClock;
      }
      mainForm["edid_block_1"].setSelection(pClock.substring(2, pClock.length));
      mainForm["edid_block_2"].setSelection(pClock.substring(0, 2));
   }
   else
   {
      mainForm["edid_block_1"].setSelection(customDTDParameters["pixel_clock"]);
      mainForm["edid_block_2"].setSelection(customDTDParameters["pixel_clock"]);
   }
   if(customDTDParameters["hztl_active"].length>0 && customDTDParameters["hztl_blanking"].length>0)
   {
      var horizActive = parseInt(parseInt(customDTDParameters["hztl_active"])).toString(16);
      while(horizActive.length<3)
      {
         horizActive = ZERO+(horizActive);
      }
      var horizBlank = parseInt(parseInt(customDTDParameters["hztl_blanking"])).toString(16);
      while(horizBlank.length<3)
      {
         horizBlank = ZERO+(horizBlank);
      }
      mainForm["edid_block_3"].setSelection(horizActive.substring(1,horizActive.length));
      mainForm["edid_block_4"].setSelection(horizBlank.substring(1,horizBlank.length));
      mainForm["edid_block_5"].setSelection(horizActive.substring(0, 1).concat(horizBlank.substring(0, 1)));
   }
   else if(customDTDParameters["hztl_active"].length>0)
   {
      var horizActive = parseInt(parseInt(customDTDParameters["hztl_active"])).toString(16);
      while(horizActive.length<3)
      {
         horizActive = ZERO+(horizActive);
      }
      mainForm["edid_block_3"].setSelection(horizActive.substring(1, horizActive.length));
      mainForm["edid_block_4"].setSelection("");
      mainForm["edid_block_5"].setSelection(horizActive.substring(0, 1)+(ZERO));
   }
   else if(customDTDParameters["hztl_blanking"].length>0)
   {
      var horizBlank = parseInt(parseInt(customDTDParameters["hztl_blanking"])).toString(16);
      while(horizBlank.length<3)
      {
         horizBlank = ZERO+(horizBlank);
      }
      mainForm["edid_block_3"].setSelection("");
      mainForm["edid_block_4"].setSelection(horizBlank.substring(1, horizBlank.length ));
      mainForm["edid_block_5"].setSelection(ZERO.concat(horizBlank.substring(0, 1)));
   }
   else
   {
      mainForm["edid_block_3"].setSelection("");
      mainForm["edid_block_4"].setSelection("");
      mainForm["edid_block_5"].setSelection("");
   }
   if(customDTDParameters["vertical_active"].length>0 && customDTDParameters["vertical_blanking"].length>0)
   {
      var vertActive = parseInt(parseInt(customDTDParameters["vertical_active"])).toString(16);
      while(vertActive.length<3)
      {
         vertActive = ZERO+(vertActive);
      }
      var vertBlank = parseInt(parseInt(customDTDParameters["vertical_blanking"])).toString(16);
      while(vertBlank.length<3)
      {
         vertBlank = ZERO.concat(vertBlank);
      }
      mainForm["edid_block_6"].setSelection(vertActive.substring(1, vertActive.length));
      mainForm["edid_block_7"].setSelection(vertBlank.substring(1, vertBlank.length));
      mainForm["edid_block_8"].setSelection(vertActive.substring(0, 1)+(vertBlank.substring(0, 1)));
   }
   else if(customDTDParameters["vertical_active"].length>0)
   {
      var vertActive = parseInt(parseInt(customDTDParameters["vertical_active"])).toString(16);
      while(vertActive.length<3)
      {
         vertActive = ZERO+(vertActive);
      }
      mainForm["edid_block_6"].setSelection(vertActive.substring(1, vertActive.length));
      mainForm["edid_block_7"].setSelection("");
      mainForm["edid_block_8"].setSelection(vertActive.substring(0, 1)+(ZERO));
   }
   else if(customDTDParameters["vertical_blanking"].length>0)
   {
      var vertBlank = parseInt(parseInt(customDTDParameters["vertical_blanking"])).toString(16);
      while(vertBlank.length<3)
      {
         vertBlank = ZERO+(vertBlank);
      }
      mainForm["edid_block_6"].setSelection("");
      mainForm["edid_block_7"].setSelection(vertBlank.substring(1, vertBlank.length));
      mainForm["edid_block_8"].setSelection(ZERO+(vertBlank.substring(0, 1)));
   }
   else
   {
      mainForm["edid_block_6"].setSelection("");
      mainForm["edid_block_7"].setSelection("");
      mainForm["edid_block_8"].setSelection("");
   }
   if(customDTDParameters["hztl_sync_offset"].length>0)
   {
      var horizSyncOffset = parseInt(parseInt(customDTDParameters["hztl_sync_offset"])).toString(16);
      if(horizSyncOffset.length>2)
      {
         horizSyncOffset = horizSyncOffset.substring(horizSyncOffset.length-2,horizSyncOffset.length);
      }
      mainForm["edid_block_9"].setSelection(horizSyncOffset);
   }
   else
   {
      mainForm["edid_block_9"].setSelection("");
   }
   if(customDTDParameters["hztl_sync_pulse_offset"].length>0)
   {
      var horizSyncWidth = parseInt(parseInt(customDTDParameters["hztl_sync_pulse_offset"])).toString(16);
      if(horizSyncWidth.length>2)
      {
         horizSyncWidth = horizSyncWidth.substring(horizSyncWidth.length-2, horizSyncWidth.length);
      }
      mainForm["edid_block_10"].setSelection(horizSyncWidth);
   }
   else
   {
      mainForm["edid_block_10"].setSelection("");
   }
   if(customDTDParameters["vertical_sync_offset"].length>0 && customDTDParameters["vertical_sync_pulse_offset"].length>0)
   {
      var vertSyncOffset = parseInt(parseInt(customDTDParameters["vertical_sync_offset"])).toString(16);
      var vertSyncWidth = parseInt(parseInt(customDTDParameters["vertical_sync_pulse_offset"])).toString(16);
      mainForm["edid_block_11"].setSelection(vertSyncOffset.substring(vertSyncOffset.length-1, vertSyncOffset.length)+(vertSyncWidth.
            substring(vertSyncWidth.length-1, vertSyncWidth.length)));
   }
   else if(customDTDParameters["vertical_sync_offset"].length>0)
   {
      var vertSyncOffset = parseInt(parseInt(customDTDParameters["vertical_sync_offset"])).toString(16);
      var vertSyncWidth = "0";
      mainForm["edid_block_11"].setSelection(vertSyncOffset.substring(vertSyncOffset.length-1, vertSyncOffset.length)+(vertSyncWidth.
            substring(vertSyncWidth.length()-1, vertSyncWidth.length)));
   }
   else if(customDTDParameters["vertical_sync_pulse_offset"].length>0)
   {
      var vertSyncOffset = "0";
      var vertSyncWidth = parseInt(parseInt(customDTDParameters["vertical_sync_pulse_offset"])).toString(16);
      mainForm["edid_block_11"].setSelection(vertSyncOffset.substring(vertSyncOffset.length-1, vertSyncOffset.length)+(vertSyncWidth.
            substring(vertSyncWidth.length-1, vertSyncWidth.length)));
   }
   else
   {
      mainForm["edid_block_11"].setSelection("");
   }
   if(customDTDParameters["pixel_clock"].length>0||customDTDParameters["hztl_active"].length>0||customDTDParameters["hztl_blanking"].length>0||customDTDParameters["vertical_active"].length>0||
         customDTDParameters["vertical_blanking"].length>0||customDTDParameters["hztl_sync_offset"].length>0||customDTDParameters["hztl_sync_pulse_offset"].length>0||customDTDParameters["vertical_sync_offset"].length>0||
         customDTDParameters["vertical_sync_pulse_offset"].length>0)
   {
      //dtd flags
      var sByte18 = 0;
      if(customDTDParameters["interlace_display"])
      {
         sByte18 = sByte18 + 10000000;
      }
      if(customDTDParameters["vsync_polarity"]==(134217728) || customDTDParameters["hsync_polarity"]==(67108864))
      {
         sByte18 = sByte18 + 11000;
      }
      if(customDTDParameters["vysnc_polarity"]==(134217728))
      {
         sByte18 = sByte18 + 100;
      }
      if(customDTDParameters["hsync_polarity"]==67108864)
      {
         sByte18 = sByte18 + 10;
      }
      mainForm["edid_block_18"].setSelection(parseInt(parseInt(sByte18+"", 2)).toString(16));
   }
}
   
function add(value1, subtract, value2, value3)
{
    var value = "";
    if(validateIntString(value1))
        value = parseInt(value1);
    
    if(subtract)
    {
        if(validateIntString(value))
            value = value - 1;
    }
    
    if(validateIntString(value2))
        value += parseInt(value2);
    if(validateIntString(value3))
        value += parseInt(value3);
    
    return value;
}

function setPreviousSelection(prevSelection)
{
    previousSelection = prevSelection;
}

function validateIntString(intString)
{
    intString = intString+"";
      if(intString == null || intString.length == 0)
      {
         return false;
      }
      if(intString.charAt(0) == '-')
      {
         intString = intString.substring(1,intString.length);
      }
      
      for(var i =0;i<intString.length;i++)
      {
        if((intString.charAt(i) < '0') || (intString.charAt(i) > '9'))
        {
            return false;
        }
      }
      return true;
}

function validateFloatString(floatString)
{
    floatString = floatString+"";
      if(floatString == null || floatString.length == 0)
      {
         return false;
      }
      if(floatString.charAt(0) == '-')
      {
         floatString = floatString.substring(1,floatString.length);
      }
      var decPoint = floatString.indexOf(".");
      if(decPoint>=0)
      {
         floatString = floatString.substring(0, decPoint)+(floatString.substring(decPoint+1, 
               floatString.length));
      }
      // check for allowable characters and numbers
      for (var ii = 0; ii < floatString.length; ii++)
      {
         if ((floatString.charAt(ii) < '0') || (floatString.charAt(ii) > '9'))
         {
            return false;
         }
      }
      return true;
}

function validateByteHexValue(selection)
{
    selection = selection+"";
    if(selection == null || selection.length == 0)
    {
       return false;
    }
    return true;
}

function parseBoolean(strBool)
{
    if(strBool == "true")
        return true;
    else if(strBool == "false")
        return false;
}

function round(integer)
{
    if(integer>0)
        return Math.floor(integer);
    else
        return Math.ceil(integer);
}

function subtract1(value)
{
    value = value+"";
    if(value != null && !(value==""))
    {
        value = (parseInt(value)-1);
    }
    return value;
}