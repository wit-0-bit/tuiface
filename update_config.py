import json

with open("src/pkjs/config.json", "r") as f:
    config = json.load(f)

for item in config[1]["items"]:
    if "options" in item:
        new_options = []
        for opt in item["options"]:
            lbl = opt.get("label", "").lower()
            if lbl in ["battery", "date", "steps"]:
                continue
            new_options.append(opt)
        item["options"] = new_options

with open("src/pkjs/config.json", "w") as f:
    json.dump(config, f, indent=2)

