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

 //This function validates if the user has entered the valid integer
function isDigit(input)
{
	var isDigit=true;
	var numericExp = /^[0-9]+$/;
	if(input.match(numericExp))
	{
		isDigit = true;
		       
	}
	else
	{
		isDigit=  false;
	}
	return isDigit;
}
//This function validates if the user has entered the valid character
function isAlphaNumeric(input)
{
 	
 	var isAlphaNumeric = true;
	var alphaExp = /^[a-zA-Z0-9]+$/;
	if(input.match(alphaExp))
	{
	    isAlphaNumeric = true;
	       
	}else
	{
		
	    isAlphaNumeric =  false;
	}
	return isAlphaNumeric;
}


function isDecimal(floatInput)

{
        var validChars=".0123456789";
	var isFloatDigit=true;
	var char;
	for (i = 0; i < floatInput.length && isFloatDigit == true; i++)
	{
		char = floatInput.charAt(i);
		if (validChars.indexOf(char) == -1)
		{
			isFloatDigit = false;
		}
	}
	
	return isFloatDigit;
	
	

}
//This function validates if the user has entered the valid hex number
function isHex(inputValue)
{
      
        var validChar="0123456789ABCDEF"; 
	inputValue = inputValue.toUpperCase(); 
	var char;
	var isHex=true;
	for(i=0;i<inputValue.length && isHex == true ; i++)
	{
		char = inputValue.charAt(i);
		if(validChar.indexOf(char)==-1)
  		{
  		  isHex = false;
  		}
  		
  		  
  	}
  	return isHex;  	
 } 
 

 
//This function validates the User Input
function validateUserInput(event)
{
	if (!event) 
		event = window.event;
	var input = event.srcElement?event.srcElement:event.target;
	var inputLbl = document.getElementById($(input).attr("label"));
	
	var minValue = $(input).attr("minValue");
	var maxValue = $(input).attr("maxValue");
		

	
	var inputTxt = inputLbl.innerHTML;
	var inputValue = input.value;
	
	var redClr = "#FF0000";
	var blkClr =  "#000000";
	var inputNumber = parseInt(inputValue);
	var maxNumber = parseInt(maxValue);
	var minNumber = parseInt(minValue);
	
	if(inputValue != "")	
	{
		if(!isDigit(inputValue)||inputNumber < minNumber || inputNumber> maxNumber)	
		{
		       
			var errorMesage = inputTxt + " must be between " + minValue+ " and " + maxValue +".";
			showDialog(errorMesage);
			inputLbl.style.color = redClr;			
			input.style.borderColor = redClr;
			input.value = ""
			input.focus();
			
		}
		else
		
		{	
		       inputLbl.style.color = blkClr;
			input.style.borderColor = "";
			
		}
		
		
	}
 }

 function validateHexUserInput(event)
 {
	if (!event) 
		event = window.event;
	
	var input = event.srcElement?event.srcElement:event.target;
	var inputLbl = document.getElementById($(input).attr("label"));
	
	var minValue = $(input).attr("minValue");
	var maxValue = $(input).attr("maxValue");
	
    	
    	var inputTxt = inputLbl.innerHTML;
	var inputValue = input.value;
	var inputNumber = parseInt(inputValue);
	var maxNumber = parseInt(maxValue);
 	var minNumber = parseInt(minValue);
 	var redClr = "#FF0000";
 	var blkClr =  "#000000";
 	
 	
 	if(inputValue != "")	{
 		if(!isHex(inputValue) || inputNumber > maxNumber || inputNumber < minNumber)
 		{
 		       var errorMesage = inputTxt + " must be between " + minValue+ " and " + maxValue + ".";
 			showDialog(errorMesage);
 			inputLbl.style.color = redClr;
 			input.style.borderColor = redClr;
			input.value = ""
 			input.focus();
 		}
 		else 
 		{ 		      
 			inputLbl.style.color = blkClr;
 			input.style.borderColor = "";
 			
 		} 		
 	}
 }
  function validateFloatUserInput(event)
  {
 	if (!event) 
 		event = window.event;
 	
 	var input = event.srcElement?event.srcElement:event.target;
 	var inputLbl = document.getElementById($(input).attr("label"));
 	
 	var minValue = $(input).attr("minValue");
 	var maxValue = $(input).attr("maxValue");
 	
   	var inputTxt = inputLbl.innerHTML;
 	var inputValue = input.value;
 	
  	var redClr = "#FF0000";
  	var blkClr =  "#000000";
  	if(inputValue != "")	{
  	
  		if(!isDecimal(inputValue) || inputValue > maxValue || inputValue < minValue){
  		       
  			var errorMesage = inputTxt + " must be between " + minValue+ " and " + maxValue;
  			showDialog(errorMesage);
  			inputLbl.style.color = redClr;
  			input.style.borderColor = redClr;
 			input.value = ""
  			input.focus();
  		}
  		else 
  		{ 		      
  			inputLbl.style.color = blkClr;
  			input.style.borderColor = "";
  			
  		} 		
  	}
 }
 
 //This function validates the User Input to check if it is a valid alphanumeric string
 function validateInputString(event)
 {
 		if (!event) 
	 		event = window.event;
	 	
	 	var input = event.srcElement?event.srcElement:event.target;
	 	var inputLbl = document.getElementById($(input).attr("label"));
	 	
	 	var inputTxt = inputLbl.innerHTML;
	 	var inputValue = input.value;
	 	var inputValueLength = inputValue.length;
	 	var minLength = $(input).attr("minLength");
 		var maxLength = $(input).attr("maxLength");
	 	
	 	
	  	var redClr = "#FF0000";
	  	var blkClr =  "#000000";
	  	if(!isAlphaNumeric(inputValue))
	  	{
	  		var errorMesage ="A "+ inputTxt + " must be entered that is between " + minLength+ " and " + maxLength + " characters.";
	  		showDialog(errorMesage);
	  		inputLbl.style.color = redClr;
	  		input.style.borderColor = redClr;
	 		input.value = ""
	  		input.focus();
	  	}
	  	else 
	  	{ 		      
	  		inputLbl.style.color = blkClr;
	  		input.style.borderColor = "";
	  			
	  	} 		
  	
 }
 
  

                                       
function showDialog(str)
{
	$( "#dialog-validate-form" ).dialog( "open" );
	$( "#dialog-validate-form" ).html( str );	
}


