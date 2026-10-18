# OpenTyrian sources of the port. Replaced: file.c (src/port/ty_file.c),
# video.c and video_scale*.c (ty_video.c), loudness.c (ty_loudness.c).
# network.c is built without WITH_NETWORK. The AdLib player (lds_play.c,
# opl.c) only renders the music on the PC (harness, ty_render.c).
OT_FILES = \
	animlib.c \
	arg_parse.c \
	backgrnd.c \
	config.c \
	config_file.c \
	demo.c \
	destruct.c \
	editship.c \
	episodes.c \
	font.c \
	fonthand.c \
	game_menu.c \
	helptext.c \
	joystick.c \
	jukebox.c \
	keyboard.c \
	logging.c \
	lvllib.c \
	lvlmast.c \
	mainint.c \
	memreader.c \
	memwriter.c \
	menus.c \
	mouse.c \
	mtrand.c \
	musmast.c \
	network.c \
	nortsong.c \
	nortvars.c \
	opentyr.c \
	palette.c \
	params.c \
	pcxload.c \
	pcxmast.c \
	picload.c \
	player.c \
	shots.c \
	sndmast.c \
	sprite.c \
	starlib.c \
	tyrian2.c \
	varz.c \
	vga256d.c \
	vga_palette.c \
	xmas.c

OT_RENDER_FILES = \
	lds_play.c \
	opl.c
