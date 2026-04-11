#include "vg_font.h"

#if defined(VG_DRIVER) && !defined(VG_ONLY)

// font (arial), total glyph (10)
static const VGubyte arial_glyph48_commands[] = {
	VG_MOVE_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, 
	VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_CLOSE_PATH, VG_MOVE_TO, VG_QUAD_TO, 
	VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_CLOSE_PATH
	
};

static const VGfloat arial_glyph48_coordinates[] = {
	0.041504f, 0.353027f, 0.041504f, 0.479980f, 0.067383f, 0.557129f, 0.093750f, 0.634766f, 
	0.145020f, 0.676758f, 0.196777f, 0.718750f, 0.274902f, 0.718750f, 0.332520f, 0.718750f, 
	0.375977f, 0.695313f, 0.419434f, 0.672363f, 0.447754f, 0.628418f, 0.476074f, 0.584961f, 
	0.492188f, 0.521973f, 0.508301f, 0.459473f, 0.508301f, 0.353027f, 0.508301f, 0.227051f, 
	0.482422f, 0.149414f, 0.456543f, 0.072266f, 0.404785f, 0.029785f, 0.353516f, -0.012207f, 
	0.274902f, -0.012207f, 0.171387f, -0.012207f, 0.112305f, 0.062012f, 0.041504f, 0.151367f, 
	0.041504f, 0.353027f, 0.131836f, 0.353027f, 0.131836f, 0.176758f, 0.172852f, 0.118164f, 
	0.214355f, 0.060059f, 0.274902f, 0.060059f, 0.335449f, 0.060059f, 0.376465f, 0.118652f, 
	0.417969f, 0.177246f, 0.417969f, 0.353027f, 0.417969f, 0.529785f, 0.376465f, 0.587891f, 
	0.335449f, 0.645996f, 0.273926f, 0.645996f, 0.213379f, 0.645996f, 0.177246f, 0.594727f, 
	0.131836f, 0.529297f, 0.131836f, 0.353027f
};

static const VGubyte arial_glyph49_commands[] = {
	VG_MOVE_TO, VG_LINE_TO, VG_LINE_TO, VG_QUAD_TO, VG_QUAD_TO, VG_LINE_TO, VG_QUAD_TO, VG_QUAD_TO, 
	VG_LINE_TO, VG_LINE_TO, VG_CLOSE_PATH
};

static const VGfloat arial_glyph49_coordinates[] = {
	0.372559f, 0.000000f, 0.284668f, 0.000000f, 0.284668f, 0.560059f, 0.252930f, 0.529785f, 
	0.201172f, 0.499512f, 0.149902f, 0.469238f, 0.108887f, 0.454102f, 0.108887f, 0.539063f, 
	0.182617f, 0.573730f, 0.237793f, 0.623047f, 0.292969f, 0.672363f, 0.315918f, 0.718750f, 
	0.372559f, 0.718750f, 0.372559f, 0.000000f
};

static const VGubyte arial_glyph50_commands[] = {
	VG_MOVE_TO, VG_LINE_TO, VG_LINE_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, 
	VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_LINE_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, 
	VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_LINE_TO, VG_CLOSE_PATH
	
};

static const VGfloat arial_glyph50_coordinates[] = {
	0.503418f, 0.084473f, 0.503418f, 0.000000f, 0.030273f, 0.000000f, 0.029297f, 0.031738f, 
	0.040527f, 0.061035f, 0.058594f, 0.109375f, 0.098145f, 0.156250f, 0.138184f, 0.203125f, 
	0.213379f, 0.264648f, 0.330078f, 0.360352f, 0.371094f, 0.416016f, 0.412109f, 0.472168f, 
	0.412109f, 0.521973f, 0.412109f, 0.574219f, 0.374512f, 0.609863f, 0.337402f, 0.645996f, 
	0.277344f, 0.645996f, 0.213867f, 0.645996f, 0.175781f, 0.607910f, 0.137695f, 0.569824f, 
	0.137207f, 0.502441f, 0.046875f, 0.511719f, 0.056152f, 0.612793f, 0.116699f, 0.665527f, 
	0.177246f, 0.718750f, 0.279297f, 0.718750f, 0.382324f, 0.718750f, 0.442383f, 0.661621f, 
	0.502441f, 0.604492f, 0.502441f, 0.520020f, 0.502441f, 0.477051f, 0.484863f, 0.435547f, 
	0.467285f, 0.394043f, 0.426270f, 0.348145f, 0.385742f, 0.302246f, 0.291016f, 0.222168f, 
	0.211914f, 0.155762f, 0.189453f, 0.131836f, 0.166992f, 0.108398f, 0.152344f, 0.084473f, 
	0.503418f, 0.084473f
};

static const VGubyte arial_glyph51_commands[] = {
	VG_MOVE_TO, VG_LINE_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, 
	VG_QUAD_TO, VG_LINE_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, 
	VG_QUAD_TO, VG_LINE_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, 
	VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_CLOSE_PATH
	
};

static const VGfloat arial_glyph51_coordinates[] = {
	0.041992f, 0.188965f, 0.129883f, 0.200684f, 0.145020f, 0.125977f, 0.181152f, 0.092773f, 
	0.217773f, 0.060059f, 0.270020f, 0.060059f, 0.332031f, 0.060059f, 0.374512f, 0.103027f, 
	0.417480f, 0.145996f, 0.417480f, 0.209473f, 0.417480f, 0.270020f, 0.377930f, 0.309082f, 
	0.338379f, 0.348633f, 0.277344f, 0.348633f, 0.252441f, 0.348633f, 0.215332f, 0.338867f, 
	0.225098f, 0.416016f, 0.233887f, 0.415039f, 0.239258f, 0.415039f, 0.295410f, 0.415039f, 
	0.340332f, 0.444336f, 0.385254f, 0.473633f, 0.385254f, 0.534668f, 0.385254f, 0.583008f, 
	0.352539f, 0.614746f, 0.319824f, 0.646484f, 0.268066f, 0.646484f, 0.216797f, 0.646484f, 
	0.182617f, 0.614258f, 0.148438f, 0.582031f, 0.138672f, 0.517578f, 0.050781f, 0.533203f, 
	0.066895f, 0.621582f, 0.124023f, 0.669922f, 0.181152f, 0.718750f, 0.266113f, 0.718750f, 
	0.324707f, 0.718750f, 0.374023f, 0.693359f, 0.423340f, 0.668457f, 0.449219f, 0.625000f, 
	0.475586f, 0.581543f, 0.475586f, 0.532715f, 0.475586f, 0.486328f, 0.450684f, 0.448242f, 
	0.425781f, 0.410156f, 0.376953f, 0.387695f, 0.440430f, 0.373047f, 0.475586f, 0.326660f, 
	0.510742f, 0.280762f, 0.510742f, 0.211426f, 0.510742f, 0.117676f, 0.442383f, 0.052246f, 
	0.374023f, -0.012695f, 0.269531f, -0.012695f, 0.175293f, -0.012695f, 0.112793f, 0.043457f, 
	0.050781f, 0.099609f, 0.041992f, 0.188965f
};

static const VGubyte arial_glyph52_commands[] = {
	VG_MOVE_TO, VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, 
	VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, VG_CLOSE_PATH, VG_MOVE_TO, VG_LINE_TO, VG_LINE_TO, 
	VG_LINE_TO, VG_CLOSE_PATH
};

static const VGfloat arial_glyph52_coordinates[] = {
	0.323242f, 0.000000f, 0.323242f, 0.171387f, 0.012695f, 0.171387f, 0.012695f, 0.251953f, 
	0.339355f, 0.715820f, 0.411133f, 0.715820f, 0.411133f, 0.251953f, 0.507813f, 0.251953f, 
	0.507813f, 0.171387f, 0.411133f, 0.171387f, 0.411133f, 0.000000f, 0.323242f, 0.000000f, 
	0.323242f, 0.251953f, 0.323242f, 0.574707f, 0.099121f, 0.251953f, 0.323242f, 0.251953f
	
};

static const VGubyte arial_glyph53_commands[] = {
	VG_MOVE_TO, VG_LINE_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, 
	VG_QUAD_TO, VG_QUAD_TO, VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, 
	VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_CLOSE_PATH
	
};

static const VGfloat arial_glyph53_coordinates[] = {
	0.041504f, 0.187500f, 0.133789f, 0.195313f, 0.144043f, 0.127930f, 0.181152f, 0.093750f, 
	0.218750f, 0.060059f, 0.271484f, 0.060059f, 0.334961f, 0.060059f, 0.378906f, 0.107910f, 
	0.422852f, 0.155762f, 0.422852f, 0.234863f, 0.422852f, 0.310059f, 0.380371f, 0.353516f, 
	0.338379f, 0.396973f, 0.270020f, 0.396973f, 0.227539f, 0.396973f, 0.193359f, 0.377441f, 
	0.159180f, 0.358398f, 0.139648f, 0.327637f, 0.057129f, 0.338379f, 0.126465f, 0.706055f, 
	0.482422f, 0.706055f, 0.482422f, 0.622070f, 0.196777f, 0.622070f, 0.158203f, 0.429688f, 
	0.222656f, 0.474609f, 0.293457f, 0.474609f, 0.387207f, 0.474609f, 0.451660f, 0.409668f, 
	0.516113f, 0.344727f, 0.516113f, 0.242676f, 0.516113f, 0.145508f, 0.459473f, 0.074707f, 
	0.390625f, -0.012207f, 0.271484f, -0.012207f, 0.173828f, -0.012207f, 0.111816f, 0.042480f, 
	0.050293f, 0.097168f, 0.041504f, 0.187500f
};

static const VGubyte arial_glyph54_commands[] = {
	VG_MOVE_TO, VG_LINE_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, 
	VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, 
	VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_CLOSE_PATH, VG_MOVE_TO, VG_QUAD_TO, VG_QUAD_TO, 
	VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_CLOSE_PATH
	
};

static const VGfloat arial_glyph54_coordinates[] = {
	0.497559f, 0.540527f, 0.410156f, 0.533691f, 0.398438f, 0.585449f, 0.376953f, 0.608887f, 
	0.341309f, 0.646484f, 0.289063f, 0.646484f, 0.247070f, 0.646484f, 0.215332f, 0.623047f, 
	0.173828f, 0.592773f, 0.149902f, 0.534668f, 0.125977f, 0.476563f, 0.125000f, 0.369141f, 
	0.156738f, 0.417480f, 0.202637f, 0.440918f, 0.248535f, 0.464355f, 0.298828f, 0.464355f, 
	0.386719f, 0.464355f, 0.448242f, 0.399414f, 0.510254f, 0.334961f, 0.510254f, 0.232422f, 
	0.510254f, 0.165039f, 0.480957f, 0.106934f, 0.452148f, 0.049316f, 0.401367f, 0.018555f, 
	0.350586f, -0.012207f, 0.286133f, -0.012207f, 0.176270f, -0.012207f, 0.106934f, 0.068359f, 
	0.037598f, 0.149414f, 0.037598f, 0.334961f, 0.037598f, 0.542480f, 0.114258f, 0.636719f, 
	0.181152f, 0.718750f, 0.294434f, 0.718750f, 0.378906f, 0.718750f, 0.432617f, 0.671387f, 
	0.486816f, 0.624023f, 0.497559f, 0.540527f, 0.138672f, 0.231934f, 0.138672f, 0.186523f, 
	0.157715f, 0.145020f, 0.177246f, 0.103516f, 0.211914f, 0.081543f, 0.246582f, 0.060059f, 
	0.284668f, 0.060059f, 0.340332f, 0.060059f, 0.380371f, 0.104980f, 0.420410f, 0.149902f, 
	0.420410f, 0.227051f, 0.420410f, 0.301270f, 0.380859f, 0.343750f, 0.341309f, 0.386719f, 
	0.281250f, 0.386719f, 0.221680f, 0.386719f, 0.180176f, 0.343750f, 0.138672f, 0.301270f, 
	0.138672f, 0.231934f
};

static const VGubyte arial_glyph55_commands[] = {
	VG_MOVE_TO, VG_LINE_TO, VG_LINE_TO, VG_LINE_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_LINE_TO, 
	VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_LINE_TO, VG_CLOSE_PATH
};

static const VGfloat arial_glyph55_coordinates[] = {
	0.047363f, 0.622070f, 0.047363f, 0.706543f, 0.510742f, 0.706543f, 0.510742f, 0.638184f, 
	0.442383f, 0.565430f, 0.375000f, 0.444824f, 0.308105f, 0.324219f, 0.271484f, 0.196777f, 
	0.245117f, 0.106934f, 0.237793f, 0.000000f, 0.147461f, 0.000000f, 0.148926f, 0.084473f, 
	0.180664f, 0.204102f, 0.212402f, 0.323730f, 0.271484f, 0.434570f, 0.331055f, 0.545898f, 
	0.397949f, 0.622070f, 0.047363f, 0.622070f
};

static const VGubyte arial_glyph56_commands[] = {
	VG_MOVE_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, 
	VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, 
	VG_QUAD_TO, VG_CLOSE_PATH, VG_MOVE_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, 
	VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_CLOSE_PATH, VG_MOVE_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, 
	VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_CLOSE_PATH
};

static const VGfloat arial_glyph56_coordinates[] = {
	0.176758f, 0.388184f, 0.122070f, 0.408203f, 0.095703f, 0.445313f, 0.069336f, 0.482422f, 
	0.069336f, 0.534180f, 0.069336f, 0.612305f, 0.125488f, 0.665527f, 0.181641f, 0.718750f, 
	0.274902f, 0.718750f, 0.368652f, 0.718750f, 0.425781f, 0.664063f, 0.482910f, 0.609863f, 
	0.482910f, 0.531738f, 0.482910f, 0.481934f, 0.456543f, 0.444824f, 0.430664f, 0.408203f, 
	0.377441f, 0.388184f, 0.443359f, 0.366699f, 0.477539f, 0.318848f, 0.512207f, 0.270996f, 
	0.512207f, 0.204590f, 0.512207f, 0.112793f, 0.447266f, 0.050293f, 0.382324f, -0.012207f, 
	0.276367f, -0.012207f, 0.170410f, -0.012207f, 0.105469f, 0.050293f, 0.040527f, 0.113281f, 
	0.040527f, 0.207031f, 0.040527f, 0.276855f, 0.075684f, 0.323730f, 0.111328f, 0.371094f, 
	0.176758f, 0.388184f, 0.159180f, 0.537109f, 0.159180f, 0.486328f, 0.191895f, 0.454102f, 
	0.224609f, 0.421875f, 0.276855f, 0.421875f, 0.327637f, 0.421875f, 0.359863f, 0.453613f, 
	0.392578f, 0.485840f, 0.392578f, 0.532227f, 0.392578f, 0.580566f, 0.358887f, 0.613281f, 
	0.325684f, 0.646484f, 0.275879f, 0.646484f, 0.225586f, 0.646484f, 0.192383f, 0.614258f, 
	0.159180f, 0.582031f, 0.159180f, 0.537109f, 0.130859f, 0.206543f, 0.130859f, 0.168945f, 
	0.148438f, 0.133789f, 0.166504f, 0.098633f, 0.201660f, 0.079102f, 0.236816f, 0.060059f, 
	0.277344f, 0.060059f, 0.340332f, 0.060059f, 0.381348f, 0.100586f, 0.422363f, 0.141113f, 
	0.422363f, 0.203613f, 0.422363f, 0.267090f, 0.379883f, 0.308594f, 0.337891f, 0.350098f, 
	0.274414f, 0.350098f, 0.212402f, 0.350098f, 0.171387f, 0.309082f, 0.130859f, 0.268066f, 
	0.130859f, 0.206543f
};

static const VGubyte arial_glyph57_commands[] = {
	VG_MOVE_TO, VG_LINE_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, 
	VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, 
	VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_CLOSE_PATH, 
	VG_MOVE_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, VG_QUAD_TO, 
	VG_QUAD_TO, VG_CLOSE_PATH
};

static const VGfloat arial_glyph57_coordinates[] = {
	0.054688f, 0.165527f, 0.139160f, 0.173340f, 0.149902f, 0.113770f, 0.180176f, 0.086914f, 
	0.210449f, 0.060059f, 0.257813f, 0.060059f, 0.298340f, 0.060059f, 0.328613f, 0.078613f, 
	0.359375f, 0.097168f, 0.378906f, 0.127930f, 0.398438f, 0.159180f, 0.411621f, 0.211914f, 
	0.424805f, 0.264648f, 0.424805f, 0.319336f, 0.424805f, 0.325195f, 0.424316f, 0.336914f, 
	0.397949f, 0.294922f, 0.352051f, 0.268555f, 0.306641f, 0.242676f, 0.253418f, 0.242676f, 
	0.164551f, 0.242676f, 0.103027f, 0.307129f, 0.041504f, 0.371582f, 0.041504f, 0.477051f, 
	0.041504f, 0.585938f, 0.105469f, 0.652344f, 0.169922f, 0.718750f, 0.266602f, 0.718750f, 
	0.336426f, 0.718750f, 0.394043f, 0.681152f, 0.452148f, 0.643555f, 0.481934f, 0.573730f, 
	0.512207f, 0.504395f, 0.512207f, 0.372559f, 0.512207f, 0.235352f, 0.482422f, 0.153809f, 
	0.452637f, 0.072754f, 0.393555f, 0.030273f, 0.334961f, -0.012207f, 0.255859f, -0.012207f, 
	0.171875f, -0.012207f, 0.118652f, 0.034180f, 0.065430f, 0.081055f, 0.054688f, 0.165527f, 
	0.414551f, 0.481445f, 0.414551f, 0.557129f, 0.374023f, 0.601563f, 0.333984f, 0.645996f, 
	0.277344f, 0.645996f, 0.218750f, 0.645996f, 0.175293f, 0.598145f, 0.131836f, 0.550293f, 
	0.131836f, 0.474121f, 0.131836f, 0.405762f, 0.172852f, 0.362793f, 0.214355f, 0.320313f, 
	0.274902f, 0.320313f, 0.335938f, 0.320313f, 0.375000f, 0.362793f, 0.414551f, 0.405762f, 
	0.414551f, 0.481445f
};

// font glyphs, sorted by ascending glyph index
static const Glyph arial_glyphs[] = {
	{    48, { 0.556152f, 0.000000f },  24, arial_glyph48_commands,  84, arial_glyph48_coordinates, VG_NON_ZERO },
	{    49, { 0.556152f, 0.000000f },  11, arial_glyph49_commands,  28, arial_glyph49_coordinates, VG_NON_ZERO },
	{    50, { 0.556152f, 0.000000f },  24, arial_glyph50_commands,  82, arial_glyph50_coordinates, VG_NON_ZERO },
	{    51, { 0.556152f, 0.000000f },  32, arial_glyph51_commands, 116, arial_glyph51_coordinates, VG_NON_ZERO },
	{    52, { 0.556152f, 0.000000f },  18, arial_glyph52_commands,  32, arial_glyph52_coordinates, VG_NON_ZERO },
	{    53, { 0.556152f, 0.000000f },  24, arial_glyph53_commands,  76, arial_glyph53_coordinates, VG_NON_ZERO },
	{    54, { 0.556152f, 0.000000f },  32, arial_glyph54_commands, 114, arial_glyph54_coordinates, VG_NON_ZERO },
	{    55, { 0.556152f, 0.000000f },  13, arial_glyph55_commands,  36, arial_glyph55_coordinates, VG_NON_ZERO },
	{    56, { 0.556152f, 0.000000f },  39, arial_glyph56_commands, 138, arial_glyph56_coordinates, VG_NON_ZERO },
	{    57, { 0.556152f, 0.000000f },  34, arial_glyph57_commands, 122, arial_glyph57_coordinates, VG_NON_ZERO }};

// the arial font structure
Font arial_font = {
	// OpenVG font object
	VG_INVALID_HANDLE,
	// glyphs data (entries are sorted by ascending glyph index)
	arial_glyphs,
	// number of glyphs
	10,
	// kerning table
	0, //arial_kerning_table,
	// number of kerning entries
	0
};

// initialize arial font
VGErrorCode arialFontInit(void) 
{
	VGErrorCode err = VG_NO_ERROR;
	const VGfloat arial_glyphs_origin[2] = { 0.0f, 0.0f };

	if (arial_font.openvgHandle == VG_INVALID_HANDLE)
	{
		// create OpenVG font object
		arial_font.openvgHandle = vgCreateFont(arial_font.glyphsCount);
		if (arial_font.openvgHandle != VG_INVALID_HANDLE) 
		{
			VGuint i;
			// create OpenVG glyphs
			for (i = 0; i < arial_font.glyphsCount; ++i) 
			{
				VGPath path = vgCreatePath (VG_PATH_FORMAT_STANDARD, 
													 VG_PATH_DATATYPE_F, 
													 1.0f, 
													 0.0f, 
													 arial_font.glyphs[i].commandsCount, 
													 arial_font.glyphs[i].coordinatesCount, 
													 VG_PATH_CAPABILITY_ALL
													);
				vgAppendPathData(path, arial_font.glyphs[i].commandsCount, arial_font.glyphs[i].commands, arial_font.glyphs[i].coordinates);
  				// remove "editing" capabilities, so that OpenVG driver can try to free some memory
				vgRemovePathCapabilities(path, VG_PATH_CAPABILITY_APPEND_FROM | VG_PATH_CAPABILITY_APPEND_TO |
                                           VG_PATH_CAPABILITY_MODIFY | VG_PATH_CAPABILITY_TRANSFORM_FROM |
                                           VG_PATH_CAPABILITY_TRANSFORM_TO | VG_PATH_CAPABILITY_INTERPOLATE_FROM |
                                           VG_PATH_CAPABILITY_INTERPOLATE_TO);
   			vgSetGlyphToPath (arial_font.openvgHandle,
										arial_font.glyphs[i].glyphIndex, 
										path, VG_FALSE, 
										(float *)arial_glyphs_origin, 
										(float *)arial_font.glyphs[i].escapement);
				vgDestroyPath(path);
 			}
		}
		// check for errors from the OpenVG driver side
		err = vgGetError();
	}

	return err;
}

// destroy arial font
void arialFontDestroy(void) 
{
	if (arial_font.openvgHandle != VG_INVALID_HANDLE) 
	{
		 VGuint i;
		 for (i = 0; i < arial_font.glyphsCount; ++i)
		 {
			 vgClearGlyph (arial_font.openvgHandle, arial_font.glyphs[i].glyphIndex);
		 }
		 vgDestroyFont(arial_font.openvgHandle);
		 arial_font.openvgHandle = VG_INVALID_HANDLE;
	}
	
}

#endif
