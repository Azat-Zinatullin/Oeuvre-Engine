#ifndef GRID_CALCULATION_HLSLI
#define GRID_CALCULATION_HLSLI

// extents of grid in world coordinates
static const float gridSize = 1000.0;

// size of one cell
static const float gridCellSize = 0.025;

// color of thin lines
static const float4 gridColorThin = float4(0.5, 0.5, 0.5, 1.0);

// color of thick lines (every tenth line)
static const float4 gridColorThick = float4(0.0, 0.0, 0.0, 1.0);

// minimum number of pixels between cell lines before LOD switch should occur. 
static const float gridMinPixelsBetweenCells = 2.0;

static const float3 pos[4] =
{
    float3(-1.0, 0.0, -1.0),
	float3(1.0, 0.0, -1.0),
	float3(1.0, 0.0, 1.0),
	float3(-1.0, 0.0, 1.0)
};

static const int indices[6] =
{
    0, 3, 2, 2, 1, 0
};

float log10(float x)
{
    return log(x) / log(10.0);
}

float satf(float x)
{
    return clamp(x, 0.0, 1.0);
}

float2 satv(float2 x)
{
    return clamp(x, float2(0.0, 0.0), float2(1.0, 1.0));
}

float max2(float2 v)
{
    return max(v.x, v.y);
}

float4 gridColor(float2 uv, float2 camPos)
{
    float2 dudv = float2(
		length(float2(ddx(uv.x), ddy(uv.x))),
		length(float2(ddx(uv.y), ddy(uv.y)))
	);

    float lodLevel = max(0.0, log((length(dudv) * gridMinPixelsBetweenCells) / gridCellSize) + 1.0);
    float lodFade = frac(lodLevel);

	// cell sizes for lod0, lod1 and lod2
    float lod0 = gridCellSize * pow(10.0, floor(lodLevel));
    float lod1 = lod0 * 10.0;
    float lod2 = lod1 * 10.0;

	// each anti-aliased line covers up to 4 pixels
    dudv *= 4.0;

	// Update grid coordinates for subsequent alpha calculations (centers each anti-aliased line)
    uv += dudv / 2.0F;

	// calculate absolute distances to cell line centers for each lod and pick max X/Y to get coverage alpha value
    float lod0a = max2(float2(1.0, 1.0) - abs(satv(fmod(uv, lod0) / dudv) * 2.0 - float2(1.0, 1.0)));
    float lod1a = max2(float2(1.0, 1.0) - abs(satv(fmod(uv, lod1) / dudv) * 2.0 - float2(1.0, 1.0)));
    float lod2a = max2(float2(1.0, 1.0) - abs(satv(fmod(uv, lod2) / dudv) * 2.0 - float2(1.0, 1.0)));

    uv -= camPos;

	// blend between falloff colors to handle LOD transition
    float4 c = lod2a > 0.0 ? gridColorThick : lod1a > 0.0 ? lerp(gridColorThick, gridColorThin, lodFade) : gridColorThin;

	// calculate opacity falloff based on distance to grid extents
    float opacityFalloff = (1.0 - satf(length(uv) / gridSize));

	// blend between LOD level alphas and scale with opacity falloff
    c.a *= (lod2a > 0.0 ? lod2a : lod1a > 0.0 ? lod1a : (lod0a * (1.0 - lodFade))) * opacityFalloff;

    return c;
}

#endif // GRID_CALCULATION_HLSLI