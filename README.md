# DesignatedMountainousTerrainTool
Interactive tool that allows the user to adjust the parameters used in determining Designated Mountainous Areas and visualize the result in various ways. Node.js is used for the serverside code, and the clientside code is just javascript, HTML, and CSS. Tiles are precomputed using python and stored as JSON.

Designated Mountainous Areas (DMAs) were defined in the 1950's by just drawing simple bounding polygons on a map. The airspace in these areas have significantly different rules, and over the years, some exceptions have been made to omit areas from the original polygons, effectively designating them as non-mountainous.

Agreeing on a modern definition to what should be designated mountainous will likely take many years, but this tool allows you to change the definition and view the result. It's fairly quick to paint the entire contiguous United States (CONUS).

This uses either the National Elevation Dataset (NED) or Digital Terrain Elevation Data as the original data.

![CONUS ICAO Definition](https://github.com/TomAgatchi/DesignatedMountainousAreaTool/images/CONUS_ICAOdef_blueRed.PNG)
