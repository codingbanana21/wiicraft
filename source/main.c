#include <grrlib.h>
#include <ogcsys.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <ogc/system.h>
#include <stdlib.h>
#include <math.h>

#include "../source/globals.h"

// Font
#include "BMfont5_png.h"
GRRLIB_texImg *tex_BMfont5;

//Images
#include "shot_png.h"
GRRLIB_texImg *tex_shot;

// colors
#define GRRLIB_BLACK  0x000000ff
#define GRRLIB_WHITE  0xffffffff
#define GRRLIB_RED    0xff0000ff
#define GRRLIB_GREEN  0x00ff00ff
#define GRRLIB_BLUE   0x0000ffff
#define GRRLIB_YELLOW 0xffff00ff

void player_input();
void draw_wiimotes();
bool is_node_visible(float node_x, float node_y, float node_z);
int nodes_pos(int x, int z, int y);
float clamp(float value, float min, float max);

bool project_running = true;

unsigned int frame = 0;
const int view_range = 8;
const int max_nodes_drawn = 4096;
const float player_speed = 0.5f;

float player_x = 256.0f;
float player_z = 256.0f;
float player_y = 64.0f;

// float player_velocity_x = 0.0f;
float player_velocity_y = 0.0f;
// float player_velocity_z = 0.0f;

float camera_rot_x = 0.0f;
float camera_rot_y = 0.0f;
float camera_look_x;
float camera_look_z;
float camera_look_y;

uint8_t *nodes;


int main(int argc, char **argv) {
    // setup
    GRRLIB_Init();
    
    // import font
    tex_BMfont5 = GRRLIB_LoadTexture(BMfont5_png);
    GRRLIB_InitTileSet(tex_BMfont5, 8, 16, 0); 

    // import images
    tex_shot = GRRLIB_LoadTexture(shot_png);
    GRRLIB_SetMidHandle(tex_shot, true);

    GRRLIB_Camera3dSettings(0.0f,3.0f,5.0f, 0,1,0, 0,-0.4,0);

    // wiimote setup
    WPAD_Init();
    WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR);

    GRRLIB_FillScreen(GRRLIB_BLACK);
    GRRLIB_Printf(250, 200, tex_BMfont5, GRRLIB_WHITE, 1, "loading...");
    GRRLIB_Render();

    nodes = malloc(MAP_SIZE * sizeof(uint8_t));

    if (nodes == NULL) {
        goto cleanup;
    }

    // fill map with air
    for (int i=0;i<MAP_SIZE;i++) {
        nodes[i] = 0;
    }

    // make map
    for (int x=0;x<32;x++) {
        for (int z=0;z<32;z++) {
            unsigned int hight = rand() % 3 + 5;

            // make grass
            nodes[nodes_pos(x, z, hight)] = 1;
            hight--;

            while (hight > 0) {
                nodes[nodes_pos(x, z, hight)] = 2;
                hight--;
            }
        }
    }
	
    while (project_running) {
        frame++;

        player_input();

        // jumping
        if (nodes[nodes_pos(player_x / 16.0f, player_z / 16.0f, player_y / 16.0f)] != 0) {
            player_velocity_y = 0.0f;
            player_y = (int)player_y;

            if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_1) {
                player_velocity_y = 1.0f;
            }
        } else {
            player_velocity_y -= 0.03f;
        }

        player_y += player_velocity_y;

        GRRLIB_Camera3dSettings(player_x, player_y, player_z, 
            0,1,0,
            player_x + camera_look_x, player_y + camera_look_y, player_z + camera_look_z);

        // draw background
		GRRLIB_2dMode();
        GRRLIB_FillScreen(0x4578d1ff);

        GRRLIB_3dMode(0.1, 1000, 75, 0,1);
        GRRLIB_SetLightAmbient(0x111111FF);
        GRRLIB_SetLightDiff(0, (guVector){player_x, 64.0f, player_z}, 50.0f, 1.0f, 0xffffffff);

        GX_SetCullMode(GX_CULL_FRONT);

        int nodes_drawn = 0;

        // draw map of nodes
        for (int x = round(player_x / 16.0f) - view_range; x < round(player_x / 16.0f) + view_range; x++) {
            for (int z = round(player_z / 16.0f) - view_range; z < round(player_z / 16.0f) + view_range; z++) {
                for (int y = round(player_y / 16.0f) - view_range; y < round(player_y / 16.0f) + view_range; y++) {
                    if (nodes[nodes_pos(x, z, y)] == 0 || nodes_drawn >= max_nodes_drawn) {
                        continue;
                    }

                    // Only draw if visible
                    if (!is_node_visible(x * 16, y * 16, z * 16)) {
                        continue;
                    }
                    
                    GRRLIB_ObjectViewBegin();
                    GRRLIB_ObjectViewRotate(0, 0, 0);
                    GRRLIB_ObjectViewScale(16, 16, 16);
                    GRRLIB_ObjectViewTrans(x * 16, y * 16, z * 16);
                    GRRLIB_ObjectViewEnd();

                    nodes_drawn++;

                    if (nodes[nodes_pos(x, z, y)] == 1) {
                        GRRLIB_DrawCube(1, 1, 0x228e13ff);
                    }
                    else if (nodes[nodes_pos(x, z, y)] == 2) {
                        GRRLIB_DrawCube(1, 1, 0x352608ff);
                    }
                    else if (nodes[nodes_pos(x, z, y)] == 3) {
                        GRRLIB_DrawCube(1, 1, 0xffffffff);
                    }  
                }
            }
        }
        // for (int x = (int)player_x - view_range; x < (int)player_x + view_range; x++) {
        //     for (int z = (int)player_z - view_range; z < (int)player_z + view_range; z++) {
        //         for (int y = (int)player_y - view_range; y < (int)player_y + view_range; y++) {
        //             uint8_t node = nodes[nodes_pos(x, z, y)];

        //             if (nodes[node] == 0 || nodes_drawn >= max_nodes_drawn) {
        //                 continue;
        //             }

        //             // Only draw if visible
        //             // if (!is_node_visible(x, z, y)) {
        //             //     continue;
        //             // }
                    
        //             GRRLIB_ObjectViewBegin();
        //             GRRLIB_ObjectViewRotate(0, 0, 0);
        //             GRRLIB_ObjectViewScale(16, 16, 16);
        //             GRRLIB_ObjectViewTrans(x, z, y);
        //             GRRLIB_ObjectViewEnd();

        //             nodes_drawn++;

        //             if (nodes[node] == 1) {
        //                 GRRLIB_DrawCube(1, 1, 0x228e13ff);
        //             } else if (nodes[node] == 2) {
        //                 GRRLIB_DrawCube(1, 1, 0x352608ff);
        //             } else if (nodes[node] == 3) {
        //                 GRRLIB_DrawCube(1, 1, 0xffffffff);
        //             }  
        //         }
        //     }
        // }

        // UI
        GX_SetCullMode(GX_CULL_NONE);
        GRRLIB_2dMode();
        draw_wiimotes();

        GRRLIB_Printf(0, 25, tex_BMfont5, GRRLIB_WHITE, 1, "Player pos x %f z %f y %f", player_x, player_z, player_y);
        GRRLIB_Printf(0, 50, tex_BMfont5, GRRLIB_WHITE, 1, "Player node pos x %d z %d y %d", (int)player_x, (int)player_z, (int)player_y);
        GRRLIB_Printf(0, 75, tex_BMfont5, GRRLIB_WHITE, 1, "Camera look pos x %f z %f y %f", camera_look_x, camera_look_z, camera_look_y);
        GRRLIB_Printf(0, 100, tex_BMfont5, GRRLIB_WHITE, 1, "Nodes drawn %d", nodes_drawn);
        GRRLIB_Render();
    }

    free(nodes);
    nodes = NULL;
    cleanup:
    GRRLIB_FreeTexture(tex_shot);
    GRRLIB_FreeTexture(tex_BMfont5);
    GRRLIB_Exit();
    return 0;
}


void player_input() {
    WPAD_ScanPads();
    WPADData* data = WPAD_Data(0);


    if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) {
        project_running = false;
    }

    if (data->ir.x > 500.0f) {
        camera_rot_x -= 0.03f;
    }
    
    if (data->ir.x < 100.0f) {
        camera_rot_x += 0.03f;
    }

    if (data->ir.y > 350.0f) {
        camera_rot_y -= 0.03f;
    }

    if (data->ir.y < 100.0f) {
        camera_rot_y += 0.03f;
    }

    camera_look_x = sin(camera_rot_x) * cos(camera_rot_y) * 2;
    camera_look_z = cos(camera_rot_x) * cos(camera_rot_y) * 2;
    camera_look_y = sin(camera_rot_y) * 2;

    if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_UP) {
        player_x += camera_look_x * player_speed;
        player_z += camera_look_z * player_speed;
    }
    
    if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_DOWN) {
        player_x -= camera_look_x * player_speed;
        player_z -= camera_look_z * player_speed;
    }

    if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_B) {
        nodes[nodes_pos((int)player_x + camera_look_x, 
            (int)player_z + camera_look_z, 
            (int)player_y + camera_look_y)] = 0;
    }

    if (WPAD_ButtonsHeld(0) & WPAD_BUTTON_A) {
        nodes[nodes_pos((int)player_x + camera_look_x, 
            (int)player_z + camera_look_z, 
            (int)player_y + camera_look_y)] = 3;
    }
}


void draw_wiimotes() {
    WPAD_ScanPads();

    for (int wiimote = 0; wiimote <= 3; wiimote++) {
        WPADData* data = WPAD_Data(wiimote);

        if(!data->data_present) {
            continue;
        }

        if (wiimote == 0) {
            GRRLIB_DrawImg(data->ir.x, data->ir.y, tex_shot, 1, 0.15, 0.15, GRRLIB_BLUE);
        } else if (wiimote == 1) {
            GRRLIB_DrawImg(data->ir.x, data->ir.y, tex_shot, 1, 0.15, 0.15, GRRLIB_RED);
        } else if (wiimote == 2) {
            GRRLIB_DrawImg(data->ir.x, data->ir.y, tex_shot, 1, 0.15, 0.15, GRRLIB_GREEN);
        } else {
            GRRLIB_DrawImg(data->ir.x, data->ir.y, tex_shot, 1, 0.15, 0.15, GRRLIB_YELLOW);
        }
    }
}

int nodes_pos(int x, int z, int y) {
    return (clamp(x, 0, 31) * 512) + (clamp(z, 0, 31) * 16) + clamp(y, 0, 15);
}


bool is_node_visible(float node_x, float node_y, float node_z) {
    // Direction from camera to node
    float dx = node_x - player_x;
    float dy = node_y - player_y;
    float dz = node_z - player_z;
    
    // Dot product with look direction
    float dot = dx * camera_look_x + dy * camera_look_y + dz * camera_look_z;
    
    return (dot > 1 && dot < 512);
}


float clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}