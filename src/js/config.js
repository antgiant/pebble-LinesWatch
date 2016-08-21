module.exports = [
	{
		"type": "radiogroup",
		"messageKey": "watch_style",
		"defaultValue": "1",
		"label": "Watch Style",
		"options": [
			{ 
				"label": "Original (Hours and Minutes only)", 
				"value": "0"
			},
			{ 
				"label": "Original + Date", 
				"value": "1"
			},
			{ 
				"label": "Original + Seconds", 
				"value": "2"
			}
		]
	},
	{
	  "type": "color",
	  "messageKey": "background_color",
	  "defaultValue": "000000",
	  "label": "Background Color",
	  "sunlight": true
	},
	{
	  "type": "color",
	  "messageKey": "foreground_color",
	  "defaultValue": "ffffff",
	  "label": "Foreground Color",
	  "sunlight": true
	},
	{
 		"type": "submit",
 		"defaultValue": "Save"
	}
];