# Sidebar Complications Architecture

This document serves as a reference for the design, purpose, and implementation details of Sidebar Complications in the Watchface.

## Concept & Philosophy

Sidebars are **not** positional toggles or settings for changing where complications are drawn. Instead, they are a specific category of complication suited for data sources that:
1. Have a clearly defined, measurable range (from 0% to 100% or progress towards a daily goal).
2. Are filled or emptied dynamically throughout the day.

### Hardcoded Sidebar Map
- **Left Sidebar**: Hardcoded to **Steps** (progress toward the user's daily step goal).
- **Right Sidebar**: Hardcoded to **Battery** (battery percentage from 0% to 100%).

## Architecture Rules & Constraints

To prevent feature creep or UX confusion, the following constraints must be strictly adhered to:
1. **No Positional Toggles**: Do not add options to move, disable, or dynamically swap sidebars via the Pebble configuration page.
2. **Design Pattern**: Sidebars represent continuous progress. Complications that are text-only or represent discrete categories (e.g., Date, AQI, UV) are not suitable for Sidebars.
3. **Data Sources**: Only data sources that represent progress bars (e.g., Steps, Active Minutes, Battery Life) should ever be considered for Sidebars.
