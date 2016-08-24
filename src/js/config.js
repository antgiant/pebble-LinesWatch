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
	  "type": "slider",
	  "messageKey": "big_animation_speed",
	  "defaultValue": 4,
	  "label": "Hours and Minutes Animation Speed",
	  "description": "Smaller is faster and easier on the battery.",
	  "min": 0,
	  "max": 100,
	  "step": 1
	},
	{
	  "type": "slider",
	  "messageKey": "date_animation_speed",
	  "defaultValue": 8,
	  "label": "Date Animation Speed",
	  "description": "Smaller is faster and easier on the battery.",
	  "min": 0,
	  "max": 100,
	  "step": 1
	},
	{
	  "type": "slider",
	  "messageKey": "seconds_animation_speed",
	  "defaultValue": 5,
	  "label": "Seconds Animation Speed",
	  "description": "Smaller is faster and easier on the battery.",
	  "min": 0,
	  "max": 10,
	  "step": 1
	},
	{
 		"type": "submit",
 		"defaultValue": "Save"
	}
];