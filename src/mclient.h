/*
 *  Copyright (C) 2005-2008, Rick Stuart
 *  http://www.mvpmc.org/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef MCLIENT_H
#define MCLIENT_H

// Version history (when I remember to do it):
//
// Started tracking w/2.0.
// Added shift key (REC) for accessing additional slimserver
// features.
// Added slimserver version checking and warning message.
// Added warning message when CLI port to slimserver isn't working.
//
// 2.1:
// Added browsing by album cover.
//
// 2.2
// Added browsing by album cover caching.
//
#define MCLIENT_VERSION_MAJOR	2
#define MCLIENT_VERSION_MINOR	2

#define SLIMSERVER_VERSION_MAJOR	7
#define SLIMSERVER_VERSION_MINOR_1	0
#define SLIMSERVER_VERSION_MINOR_2	1

#define SLIMSERVER_VERSION_COMPOSIT	((SLIMSERVER_VERSION_MAJOR * 10000) + (SLIMSERVER_VERSION_MINOR_1 * 100) + SLIMSERVER_VERSION_MINOR_2)
#define SLIMSERVER_VERSION_COMPOSIT_OLD_1	((6 * 10000) + (5 * 100) + 4)
#define SLIMSERVER_VERSION_COMPOSIT_OLD_2	((6 * 10000) + (3 * 100) + 0)

#define MCLIENT_DISABLE		0
#define MCLIENT			1
#define MCLIENT_OTHER		2       /* Maybe someday UPNP? */

#define SERVER_PORT    3483

#define CLIENT_PORT    34443

#define STATE_STARTUP 0

#define RECV_BUF_SIZE 65536
#define OUT_BUF_SIZE  131072
#define DISPLAY_SIZE 128
#define LINE_LENGTH 40
#define LINE_COUNT 2

#define CLI_PORT    9090
#define RECV_BUF_SIZE_CLI 32768
#define CLI_MAX_TRACKS 8

#define MAX_BUTTON_SCANS 10

/*
 * Command Line Interface Defines.
 */
#define MAX_ID_SIZE 32
#define MAX_CMD_SIZE 1024
#define MAX_PARAM_SIZE 64
#define MAX_PARAMS 64
#define MAX_REPLY_LENGTH 256

#define FALSE 0
#define TRUE 1

/*
 * Define states of the cli process.
 */
enum {
    UPDATE_PLAYLIST_MINMINUS1 = 0,
    UPDATE_PLAYLIST_NOWPLAYING,
    UPDATE_PLAYLIST_INDEX,
    UPDATE_PLAYLIST_NUM_TRACKS,
    UPDATE_PLAYLIST_ARTIST,
    UPDATE_PLAYLIST_ALBUM,
    UPDATE_PLAYLIST_TITLE,
    UPDATE_PLAYLIST_MAXPLUS1,
    UPDATE_RADIO_MINMINUS1,
    UPDATE_RADIO_STATION,
    UPDATE_RADIO_NOWPLAYING,
    UPDATE_RADIO_MAXPLUS1,
};

/*
 * Define mclient's small widget states.
 */
enum {
    SHOW = 0,
    HIDE,
};

/*
 * Define mclient's large widget states.
 */
enum {
    UNINITIALIZED = 0,
    INITIALIZED,
    PLAY_LISTS,
    STREAMING_RADIO,
};

/*
 * Define current state to display on OSD.
 */
enum {
    MODE_UNINITIALIZED = 0,
    STOP,
    PLAY,
    PAUSE,
    STREAMING,
};

/*
 * Define last button press direction.
 */
enum {
    DIR_CLEARED = 0,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    SERVICED,
};

/*
 * Define slimserver menu states.
 */
enum {
    UNKNOWN = 0,
    SLIMP3_HOME,
    PLAYLIST,

    NOW_PLAYING,
    BROWSE_MUSIC,
    SEARCH_MUSIC,
    RANDOM_MIX,
    BROWSE_PLAYLISTS,
    INTERNET_RADIO,
    SETTINGS,
    PLUGINS,
};

/*
 * Define slimserver get cover art states.
 */
enum {
    IDLE_COVERART = 0,
    GET_1ST_ALBUM_COVERART,
    GET_1ST_TRACK_COVERART,
    GET_2ND_ALBUM_COVERART,
    GET_2ND_TRACK_COVERART,
    GET_3RD_ALBUM_COVERART,
    GET_3RD_TRACK_COVERART,
    GET_4TH_ALBUM_COVERART,
    GET_4TH_TRACK_COVERART,
    GET_5TH_ALBUM_COVERART,
    GET_5TH_TRACK_COVERART,
    GET_6TH_ALBUM_COVERART,
    GET_6TH_TRACK_COVERART,
    DISPLAY_1ST_COVER_COVERART,
    DISPLAY_2ND_COVER_COVERART,
    DISPLAY_3RD_COVER_COVERART,
    DISPLAY_4TH_COVER_COVERART,
    DISPLAY_5TH_COVER_COVERART,
    DISPLAY_6TH_COVER_COVERART,
    GET_TOTAL_NUM_ALBUMS,
    LAST_STATE_COVERART,
};

/*
 * Define slimserver cover art update widget states.
 */
enum {
    IDLE_COVERART_WIDGET = 0,
    ADJ_COVERART_WIDGET,
    LAST_STATE_COVERART_WIDGET,
};

/*
 * Define slimserver get cover art states.
 */
enum {
    IDLE_CONTROL_ANOTHER = 0,
    WE_ARE_CONTROLLING_ANOTHER,
    LAST_STATE_CONTROL_ANOTHER,
};

/*
 * Define states of remote control button pushing.
 */
enum {
    NO_RECENT_BUTTONS_PUSHED = 0,       // After about 1.5 seconds after last release.
    PUSHING_BUTTON,             // Seen button code on this scan.
    RELEASING_BUTTON,           // Counting scans w/o button code.
    RELEASED_BUTTON,            // Reached num of scans w/o button code.
};

typedef struct {
    char type;
    char reserved1[5];
    unsigned short wptr;
    unsigned short rptr;
    unsigned short seq;
    char mac_addr[6];
} packet_ack;

typedef struct {
    void *buf;
    int head;
    int tail;
    int size;
    int playmode;
} ring_buf;

extern ring_buf *outbuf;

typedef struct {
    char type;
    char reserved1;
    unsigned short wptr;
    unsigned short rptr;
    char reserved2[12];
} request_data_struct;

typedef struct {
    char type;
    char zero;
    /* optimizer wants to put two bytes here */
    unsigned long time;         /* since startup, in 625kHz ticks = 0.625 * microsecs */
    char codeset;               /* 0xff for JVC */
    char bits;                  /* 16 for JVC */
    /* optimizer wants to put two bytes here */
    unsigned long code;
    char mac_addr[6];
} __attribute__ ((packed)) send_ir_struct;      /* be smarter than the optimizer */

typedef struct {
    char type;
    char control;
    char reserved1[4];
    unsigned short wptr;
    char reserved2[2];
    unsigned short seq;
    char reserved3[6];
} receive_mpeg_header;

typedef struct {
    char player_id[MAX_ID_SIZE];
    char cmd[MAX_CMD_SIZE];
    char param[MAX_PARAMS][MAX_PARAM_SIZE];
} mclient_cmd;

/*
 * Stores cli data and states.
 */
typedef struct {
    int state;                  // State we are in - what are we asking for over the CLI.
    int tracks;                 // Tracks on disk.
    int index_playing;          // Track we are playing.
    int index_userfocus;        // Track user focus (i.e. what user may select next).
    int index_info;             // Track we are gather information on.
    int index_line;             // Line we are printing title on.
    char artist[50];
    char album[50];
    char titles[50];
    char title_history[10][50];
    int slimserver_menu_state;
    int short_update_timer;     // Tracks cli update interval which should occur every few seconds.
    int short_update_timer_expired;     // Marks cli updates which should occur every few seconds.
    int percent;                // Tracks song progress from 0 to 100%.
    int elapsed_time;
    int total_time;
    int volume;
    int get_cover_art_holdoff_timer;
    int get_cover_art_later;
    bool cli_comm_err_mask;     // True if we want to mute the cli_comm_err flag.
    unsigned int state_for_cover_art;   // State we are in - what are we asking for over the CLI.
    unsigned int state_for_cover_art_widget;
    unsigned int album_index_for_cover_art;
    unsigned int album_index_1_of_6_for_cover_art;
    unsigned int album_start_index_for_cover_art;
    unsigned int album_max_index_for_cover_art;
    unsigned int album_index_plus_start_for_cover_art[6];
    int album_id_for_cover_art[6];
    char album_name_for_cover_art[6][51];
    char artist_name_for_cover_art[6][21];
    unsigned int track_id_for_cover_art[6];
    bool pending_proc_for_cover_art;
    bool pending_proc_for_cover_art_widget;
    bool trigger_proc_for_cover_art;
    bool outstanding_cli_message;
    unsigned int outstanding_cli_message_timer;
    unsigned int row_for_cover_art;
    unsigned int col_for_cover_art;
    unsigned int rescanning_in_progress;
    unsigned int artwork_track_id[6];
    bool check_server_version;  // Only ask server version once at start of application.
    int check_server_version_timer;     // Tracks back off asking until server is ready.
    int slim_composit_ver;      // SlimServer/SqueezeBox composit version.
} cli_data_type;

/*
 *
 */
typedef struct {
    int last_pressed;
    int elapsed_time;
    int state;
    int number_of_scans;
    int number_of_pushes;
    int shift;
    int shift_time;
    bool local_menu;
    bool local_menu_browse;
    bool local_menu_remote;
} remote_buttons_type;

/*
 * Functions:
 */
extern int music_client(void);
extern void mclient_idle_callback(mvp_widget_t *);
extern void mclient_localmenu_callback(mvp_widget_t *, char);
extern unsigned long curses2ir(int);
extern void mclient_exit(void);
extern void mclient_local_init(void);
extern int mclient_server_connect(void);
extern void receive_volume_data(unsigned short *, int);
extern void read_packet(int);
extern ring_buf *ring_buf_create(int);
extern void receive_mpeg_data(int, receive_mpeg_header *, int);
extern void send_ack(int, unsigned short);
extern void send_discovery(int);
extern void cli_init(void);
extern void *mcg_socket_reader_thread(void *);
extern void cli_send_discovery(int);
extern void cli_update_playlist(int socket);
extern void cli_send_packet(int, char *);
extern int cli_read_message(int, char *);
extern void cli_read_data(int);
extern mvp_widget_t *mclient;
extern char *mclient_server;
extern int cli_server_connect(void);
extern void cli_pick_starting_index(void);
extern void cli_decode_response(int, char *, mclient_cmd *);
extern void cli_parse_playlist(mclient_cmd *);
extern void cli_parse_display(mclient_cmd *);
extern void cli_parse_players(mclient_cmd *);
extern void cli_parse_button(mclient_cmd *);
extern void cli_parse_response(int, mclient_cmd *);
extern void cli_parse_parameters(mclient_cmd *, char **);
extern void cli_get_cover_art(void);
extern void mclient_browse_by_cover(void);
extern void mclient_browse_by_cover_widget(void);
extern void mclient_localmenu_hide_all_widgets(void);
extern int fetch_cover_image(char *, char *);
extern void mclient_get_browser_cover_art(int, mvp_widget_t *, mvp_widget_t *);
extern void send_ir(int, char, unsigned long, int);

/*
 * Track which music server is being used (none, slim, UPNP,...)
 */
extern int mclient_type;

/*
 * Need the MAC to uniquely identify this mvpmc box to
 * mclient server.
 */
extern unsigned char *mac_address_ptr;

extern pthread_mutex_t mclient_mutex;

extern int mclient_socket;

extern struct in_addr *server_addr_mclient;

extern struct timeval uptime;   /* time we started */

extern void *recvbuf;

extern char *recvbuf_back;

extern char slimp3_display[];

/*
 * Tracks states of cli interface.
 */
extern cli_data_type cli_data;

/*
 * Tracks timeout of userfocus selection.
 */
extern int cli_userfocus_timeout;

/*
 * Tracks timeout of small mclient widget.
 */
extern int cli_small_widget_timeout;
extern int cli_small_widget_state;
extern int cli_small_widget_force_hide;
extern int cli_fullscreen_widget_state;

/*
 * Tracks delays between identical CLI states.
 */
extern int cli_identical_state_interval_timer;

/*
 * Tracks when user button press requires resetting audio hardware buffer.
 */
extern int reset_mclient_hardware_buffer;

/*
 * Tracks current & old state to display on OSD.
 */
extern int mclient_display_state;
extern int mclient_display_state_old;

/*
 * Tracks the switch from "Up Next" to "Now Playing".
 */
extern int now_playing_timeout;

/*
 * Tracks FF state.
 */
extern int ffwd_state;

/*
 *
 */
extern char encoded_player_id[32];
extern char decoded_player_id[32];

/*
 * Passes CLI commands from outside socket_handle_cli space.
 */
extern char pending_cli_string[MAX_CMD_SIZE];

/*
 * Track remote control button activity.
 */
extern remote_buttons_type remote_buttons;

/*
 * Track last n (100?) locally cached album cover names.
 */
#define CACHED_ALBUM_COVERS_MAX 100
extern char cached_album_covers_names[CACHED_ALBUM_COVERS_MAX][50];
extern unsigned int cached_album_covers_head;
extern unsigned int cached_album_covers_tail;

#endif /* MCLIENT_H */
