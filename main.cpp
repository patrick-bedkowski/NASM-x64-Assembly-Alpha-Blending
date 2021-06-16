#include <iostream>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>

// simple macro for displaying messages
#define PRINT(x) std::cout << (x) << std::endl

extern "C" int alphaFunc(void* data, void* data2, unsigned int x, unsigned int y, unsigned int width, unsigned int height, double sinus_ratio);

/* BITMAP SIZE STRUCT */

struct bitmapSize{
    int height;
    int width;
    std::string name;
    bitmapSize(const int &in_height, const int &in_width, const std::string &in_name) noexcept:
        height(in_height),
        width(in_width),
        name(in_name)
    {}

    bool const operator!=(const bitmapSize& bitmap2) noexcept
    {
        return (this->height != bitmap2.height || this->width != bitmap2.width);
    }
    friend std::ostream& operator <<(std::ostream& output, bitmapSize &bmp);
};

std::ostream& operator <<(std::ostream& output, bitmapSize &bmp) {
    /* Returns stream with formatted date */
    output << "Bitmap " << bmp.name << ":" << std::endl;
    output << "Height: " << bmp.height << std::endl;
    output << "Width: " << bmp.width;
    return output;
}

/*
        |=============================|
        |       DRAW ALGORITHM        |
        |=============================|
*/

void draw(ALLEGRO_BITMAP *pBitmap, ALLEGRO_BITMAP *pBitmap2, unsigned int x, unsigned int y, double sinus_ratio)
{
    /*
        Parameters:
        pBitmap: pointer to the first/main bitmap
        pBitmap2: pointer to the second bitmap
        x: x coordinate
        y: y coordinate
        sinus_ratio: additional sinus argument
     */

    /*  LOCK BOTH BITMAPS MANUALLY
        this is required if we want to edit it in NASM
        ALLEGRO_LOCKED_REGION structure represents the locked region of the bitmap
        typedef struct ALLEGRO_LOCKED_REGION {
            void *data;     -> points to the leftmost pixel in the first row
            int format;     -> indicates the pixel format of the data
            int pitch;      -> gives the size in bytes of a single row
            int pixel_size; -> is the number of bytes used to represent a single pixel
        } ALLEGRO_LOCKED_REGION;
     */
    ALLEGRO_LOCKED_REGION* sourceRegion; // pointer to a locked region
    sourceRegion = al_lock_bitmap(pBitmap, ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA, ALLEGRO_LOCK_READWRITE);
    if (sourceRegion == nullptr)
    {
        printf("Cannot lock source bitmap.\n");
        return ;
    }

    ALLEGRO_LOCKED_REGION* outputRegion;
    outputRegion = al_lock_bitmap(pBitmap2, ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA, ALLEGRO_LOCK_READWRITE);
    if (outputRegion == nullptr)
    {
        al_unlock_bitmap(pBitmap2);
        printf("Cannot lock output bitmap.\n");
        return ;
    }

    void* data1;
    data1 = (sourceRegion->data);

    void* data2;
    data2 = (outputRegion->data); // get the leftmost pixel in the first row of the secondary image

    printf("\nsourceRegion: x: %d, y: %d, pitch: %d, pixel_size: %d\n", x, y, -sourceRegion->pitch, sourceRegion->pixel_size);
    printf("outputReg: x: %d, y: %d, pitch: %d, pixel_size: %d\n", x, y, -outputRegion->pitch, outputRegion->pixel_size);

    unsigned int width = al_get_bitmap_width(pBitmap);
    unsigned int height = al_get_bitmap_width(pBitmap);

    // Assembly function call
    std::cout <<"Sine ratio " << sinus_ratio << std::endl;
    alphaFunc(data1, data2, x, y, width, height, sinus_ratio);

    al_unlock_bitmap(pBitmap);
    al_unlock_bitmap(pBitmap2);
}

/*
        |============================|
        |        MAIN FUNCTION       |
        |============================|
*/

int main(int argc, char *argv[])
{
    ALLEGRO_DISPLAY *display; // display pointer
    ALLEGRO_BITMAP *bitmap1;
    ALLEGRO_BITMAP *bitmap2;
    ALLEGRO_EVENT_QUEUE *eventQueue;
    bool shouldProgramRun = true;
    double sinus_ratio = 2.0; // sinus argument

    if (!al_init()) // if error occurred during window initialization
    {
        printf("Allegro5 Module initialization error.\n");
        return -1;
    }

    /*
        |===========================|
        |       LOAD MODULES        |
        |===========================|
     */

    if (!al_init_image_addon()) // necessary to load bitmaps
    {
        PRINT("Module initialization error (image addon).\n");
        return -1;
    }
    if (!al_install_keyboard()) // necessary to load keyboard driver
    {
        PRINT("Module initialization error (keyboard).\n");
        return -1;
    }

    if (!al_install_mouse()) // necessary to load mouse driver
    {
        PRINT("Module initialization error (mouse).\n");
        return -1;
    }

    eventQueue = al_create_event_queue(); // create empty event queue
    if (eventQueue == nullptr) // or NULL
    {
        PRINT("Event queue initialization error.\n");
        return -1;
    }
    al_register_event_source(eventQueue, al_get_keyboard_event_source());
    al_register_event_source(eventQueue, al_get_mouse_event_source());

    /*
        |===========================|
        |       LOAD BITMAPS        |
        |===========================|
     */

    char *bitmap1_name;
    char *bitmap2_name;

    // GET BITMAPS NAME FROM THE PROGRAM ARGUMENTS
    if(argc < 3){ // if number of inserted arguments less than 2
        PRINT("Program was initialized with no bitmaps!");
        abort();
    }
    else{ // set name of the bitmaps
        bitmap1_name = argv[1];
        bitmap2_name = argv[2];
    }

    bitmap1 = al_load_bitmap(bitmap1_name);
    bitmap2 = al_load_bitmap(bitmap2_name);

    /* CHECK IF bitmaps have been created successfully */

    if (bitmap1 == nullptr) // or NULL
    {
        PRINT("File \"test.bmp\" has not been loaded successfully.\n"); // change name
        return -1;
    }

    if (bitmap2 == nullptr) // or NULL
    {
        PRINT("File \"test2.bmp\" has not been loaded successfully.\n"); // change name
        return -1;
    }

    /* FILES NEED TO HAVE EQUAL SIZES */
    bitmapSize bitmapSize1 = bitmapSize(al_get_bitmap_height(bitmap1), al_get_bitmap_width(bitmap1), "bitmap1");
    bitmapSize bitmapSize2 = bitmapSize(al_get_bitmap_height(bitmap2), al_get_bitmap_width(bitmap2), "bitmap2");

    if(bitmapSize1 != bitmapSize2){ // change this so they are not equal
        PRINT("Bitmaps have different resolutions!");
        PRINT(bitmapSize1);
        PRINT(bitmapSize2);
        return 0;
    }

    // create a display with the size of an imported image
    display = al_create_display(bitmapSize1.width, bitmapSize1.height);
    if (display == nullptr)
    {
        printf("Display initialization error.\n");
        return -1;
    }

    // draw first bitmap and display it
    al_draw_bitmap(bitmap1, 0, 0, 0);
    al_flip_display();

    /*
        |=============================|
        |       MAIN ALGORITHM        |
        |=============================|
     */

    while (shouldProgramRun)
    {
        ALLEGRO_EVENT event; // initialize new event
        al_wait_for_event(eventQueue, &event);

        if (event.type == ALLEGRO_EVENT_KEY_DOWN) // If key press has been detected
        {
            // CHECK WHICH KEY HAS BEEN PRESSED
            if (event.keyboard.keycode == ALLEGRO_KEY_SPACE){ // SPACE bar has been clicked
                shouldProgramRun = false;
            }
            else if (event.keyboard.keycode == ALLEGRO_KEY_DOWN){ // if DOWN arrow key has been clicked
                sinus_ratio -= 0.5;
                if(sinus_ratio <= 0.0){
                    sinus_ratio = 0.5;
                    std::cout << "Minimum value is: " << sinus_ratio << std::endl;
                }
                else{
                    std::cout << "Sine argument decreased to: " << sinus_ratio << std::endl;
                }
            }
            else if (event.keyboard.keycode == ALLEGRO_KEY_UP){ // if UP arrow key has been clicked
                sinus_ratio += 0.5;
                std::cout << "Sine argument increased to: " << sinus_ratio << std::endl;
            }
            else if (event.keyboard.keycode == ALLEGRO_KEY_LEFT || event.keyboard.keycode == ALLEGRO_KEY_RIGHT ){ // if UP arrow key has been clicked

            }
        }
        // If mouse click has been detected
        else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN)
        {
            // sinus argument cannot be less or equal to zero
            if(sinus_ratio <= 0.0){
                sinus_ratio = 0.5;
            }
            draw(bitmap1, bitmap2, event.mouse.x, event.mouse.y, sinus_ratio);

            al_draw_bitmap(bitmap1, 0, 0, 0); // draw bitmap again
            al_flip_display();  // update bitmap
            al_clear_to_color(al_map_rgba(0, 0, 0, 1)); // clear the depth buffer
        }
    }

    PRINT("Program has been terminated!");
    al_destroy_display(display); // destroy bitmap
    al_destroy_bitmap(bitmap1); // destroy image bitmap

    return 0; // close program
}