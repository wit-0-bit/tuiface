import json

with open("src/pkjs/config.json", "r") as f:
    config = json.load(f)

# Remove sidebar position section entirely
config[1]["items"] = [item for item in config[1]["items"] if item.get("messageKey") != "SIDEBAR_POSITION"]

# Clean up options
disallowed_labels = ["Day Name", "Sunrise", "Sunset", "High Temp", "Low Temp", "High/Low Temp", "AQI", "UV Index", "AQI / UV", "UTC Offset"]

for item in config[1]["items"]:
    if "options" in item:
        new_options = []
        for opt in item["options"]:
            # If bottom slot, don't allow Weather (label starts with "Weather")
            if item.get("messageKey") in ["SLOT_3", "SLOT_4", "SLOT_5"]:
                if opt.get("label", "").startswith("Weather"):
                    continue
            if opt.get("label") not in disallowed_labels:
                new_options.append(opt)
        item["options"] = new_options

with open("src/pkjs/config.json", "w") as f:
    json.dump(config, f, indent=2)

