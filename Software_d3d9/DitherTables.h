#pragma once

// Some 4x4 tables for performing color dithering before being written to rendertargets

static const float uniform2bit[4][4] =
{
	{ (0.0f / 16.0f - 8.0f / 16.0f) / 4.0f, (8.0f / 16.0f - 8.0f / 16.0f) / 4.0f, (2.0f / 16.0f - 8.0f / 16.0f) / 4.0f, (10.0f / 16.0f - 8.0f / 16.0f) / 4.0f },
	{ (12.0f / 16.0f - 8.0f / 16.0f) / 4.0f, (4.0f / 16.0f - 8.0f / 16.0f) / 4.0f, (14.0f / 16.0f - 8.0f / 16.0f) / 4.0f, (6.0f / 16.0f - 8.0f / 16.0f) / 4.0f },
	{ (3.0f / 16.0f - 8.0f / 16.0f) / 4.0f, (11.0f / 16.0f - 8.0f / 16.0f) / 4.0f, (1.0f / 16.0f - 8.0f / 16.0f) / 4.0f, (9.0f / 16.0f - 8.0f / 16.0f) / 4.0f },
	{ (15.0f / 16.0f - 8.0f / 16.0f) / 4.0f, (7.0f / 16.0f - 8.0f / 16.0f) / 4.0f, (13.0f / 16.0f - 8.0f / 16.0f) / 4.0f, (5.0f / 16.0f - 8.0f / 16.0f) / 4.0f }
};

static const float uniform3bit[4][4] =
{
	{ (0.0f / 16.0f - 8.0f / 16.0f) / 8.0f, (8.0f / 16.0f - 8.0f / 16.0f) / 8.0f, (2.0f / 16.0f - 8.0f / 16.0f) / 8.0f, (10.0f / 16.0f - 8.0f / 16.0f) / 8.0f },
	{ (12.0f / 16.0f - 8.0f / 16.0f) / 8.0f, (4.0f / 16.0f - 8.0f / 16.0f) / 8.0f, (14.0f / 16.0f - 8.0f / 16.0f) / 8.0f, (6.0f / 16.0f - 8.0f / 16.0f) / 8.0f },
	{ (3.0f / 16.0f - 8.0f / 16.0f) / 8.0f, (11.0f / 16.0f - 8.0f / 16.0f) / 8.0f, (1.0f / 16.0f - 8.0f / 16.0f) / 8.0f, (9.0f / 16.0f - 8.0f / 16.0f) / 8.0f },
	{ (15.0f / 16.0f - 8.0f / 16.0f) / 8.0f, (7.0f / 16.0f - 8.0f / 16.0f) / 8.0f, (13.0f / 16.0f - 8.0f / 16.0f) / 8.0f, (5.0f / 16.0f - 8.0f / 16.0f) / 8.0f }
};

static const float uniform4bit[4][4] =
{
	{ (0.0f / 16.0f - 8.0f / 16.0f) / 16.0f, (8.0f / 16.0f - 8.0f / 16.0f) / 16.0f, (2.0f / 16.0f - 8.0f / 16.0f) / 16.0f, (10.0f / 16.0f - 8.0f / 16.0f) / 16.0f },
	{ (12.0f / 16.0f - 8.0f / 16.0f) / 16.0f, (4.0f / 16.0f - 8.0f / 16.0f) / 16.0f, (14.0f / 16.0f - 8.0f / 16.0f) / 16.0f, (6.0f / 16.0f - 8.0f / 16.0f) / 16.0f },
	{ (3.0f / 16.0f - 8.0f / 16.0f) / 16.0f, (11.0f / 16.0f - 8.0f / 16.0f) / 16.0f, (1.0f / 16.0f - 8.0f / 16.0f) / 16.0f, (9.0f / 16.0f - 8.0f / 16.0f) / 16.0f },
	{ (15.0f / 16.0f - 8.0f / 16.0f) / 16.0f, (7.0f / 16.0f - 8.0f / 16.0f) / 16.0f, (13.0f / 16.0f - 8.0f / 16.0f) / 16.0f, (5.0f / 16.0f - 8.0f / 16.0f) / 16.0f }
};

static const float uniform5bit[4][4] =
{
	{ (0.0f / 16.0f - 8.0f / 16.0f) / 32.0f, (8.0f / 16.0f - 8.0f / 16.0f) / 32.0f, (2.0f / 16.0f - 8.0f / 16.0f) / 32.0f, (10.0f / 16.0f - 8.0f / 16.0f) / 32.0f },
	{ (12.0f / 16.0f - 8.0f / 16.0f) / 32.0f, (4.0f / 16.0f - 8.0f / 16.0f) / 32.0f, (14.0f / 16.0f - 8.0f / 16.0f) / 32.0f, (6.0f / 16.0f - 8.0f / 16.0f) / 32.0f },
	{ (3.0f / 16.0f - 8.0f / 16.0f) / 32.0f, (11.0f / 16.0f - 8.0f / 16.0f) / 32.0f, (1.0f / 16.0f - 8.0f / 16.0f) / 32.0f, (9.0f / 16.0f - 8.0f / 16.0f) / 32.0f },
	{ (15.0f / 16.0f - 8.0f / 16.0f) / 32.0f, (7.0f / 16.0f - 8.0f / 16.0f) / 32.0f, (13.0f / 16.0f - 8.0f / 16.0f) / 32.0f, (5.0f / 16.0f - 8.0f / 16.0f) / 32.0f }
};

static const float uniform6bit[4][4] =
{
	{ (0.0f / 16.0f - 8.0f / 16.0f) / 64.0f, (8.0f / 16.0f - 8.0f / 16.0f) / 64.0f, (2.0f / 16.0f - 8.0f / 16.0f) / 64.0f, (10.0f / 16.0f - 8.0f / 16.0f) / 64.0f },
	{ (12.0f / 16.0f - 8.0f / 16.0f) / 64.0f, (4.0f / 16.0f - 8.0f / 16.0f) / 64.0f, (14.0f / 16.0f - 8.0f / 16.0f) / 64.0f, (6.0f / 16.0f - 8.0f / 16.0f) / 64.0f },
	{ (3.0f / 16.0f - 8.0f / 16.0f) / 64.0f, (11.0f / 16.0f - 8.0f / 16.0f) / 64.0f, (1.0f / 16.0f - 8.0f / 16.0f) / 64.0f, (9.0f / 16.0f - 8.0f / 16.0f) / 64.0f },
	{ (15.0f / 16.0f - 8.0f / 16.0f) / 64.0f, (7.0f / 16.0f - 8.0f / 16.0f) / 64.0f, (13.0f / 16.0f - 8.0f / 16.0f) / 64.0f, (5.0f / 16.0f - 8.0f / 16.0f) / 64.0f }
};