#include <stdio.h>

#include "base_inc.h"
#include "win32_base_inc.h"
#include "json_parser.h"
#include "clock.h"

static Arena* pm = make_arena(GB(10));
static Arena* tm = make_arena(GB(10));

struct Pair{
    f64 x0;
    f64 y0;
    f64 x1;
    f64 y1;
};

static f64 square(f64 a){
    f64 result = (a*a);
    return(result);
}

// NOTE(casey): EarthRadius is generally expected to be 6372.8
static f64
reference_haversine(Pair* pair, f64 earth_radius){
    /* NOTE(casey): This is not meant to be a "good" way to calculate the Haversine distance.
       Instead, it attempts to follow, as closely as possible, the formula used in the real-world
       question on which these homework exercises are loosely based.
    */

    f64 lat1 = pair->y0;
    f64 lat2 = pair->y1;
    f64 lon1 = pair->x0;
    f64 lon2 = pair->x1;

    f64 d_lat = deg_to_rad_f64(lat2 - lat1);
    f64 d_lon = deg_to_rad_f64(lon2 - lon1);
    lat1 = deg_to_rad_f64(lat1);
    lat2 = deg_to_rad_f64(lat2);

    f64 a = square_f64(sin(d_lat/2.0)) + cos(lat1)*cos(lat2) * square_f64(sin(d_lon/2));
    f64 c = 2.0*asin(sqrt(a));

    f64 result = earth_radius * c;

    return(result);
}

static void
write_json_data(String8 cwd, String8 filename, u64 count){
    String8 first_line = str8_literal("{\"pairs\": [\n");
    String8 last_line = str8_literal("]}");

    File file = os_file_open(cwd, filename, 1);
    if(file.handle != INVALID_HANDLE_VALUE){
        os_file_write(&file, first_line.str, first_line.size);

        for(u64 i=0; i < count; ++i){
            f64 x0 = random_range_f64(360) - 180.0;
            f64 x1 = random_range_f64(360) - 180.0;
            f64 y0 = random_range_f64(180) - 90.0;
            f64 y1 = random_range_f64(180) - 90.0;

            ScratchArena scratch = begin_scratch(0);
            const char* line_sep = (i == count - 1) ? "\n" : ",\n";
            String8 data = str8_formatted(scratch.arena, "        {\"x0\": %f, \"y0\": %f, \"x1\": %f, \"y1\": %f}%s", x0, y0, x1, y1, line_sep);
            os_file_write(&file, data.str, data.size);
            end_scratch(scratch);
        }
        os_file_write(&file, last_line.str, last_line.size);
        os_file_close(&file);
    }
}

static u32 brace_count = 0;
static void
print_element(JsonElement* element){
    if(element->key.size > 0){
        //print("\n");
        //for(u32 i=0; i < brace_count; ++i){
        //    print("\t");
        //}
        print("%.*s: ", element->key.size, element->key.str);
    }

    if(element->type == ElementType_Null){
        print("%i", element->i);
    }
    else if(element->type == ElementType_Bool){
        if(element->b){
            print("true");
        }
        else{
            print("false");
        }
    }
    else if(element->type == ElementType_Literal){
        print("%.*s", element->lit.size, element->lit.str);
    }
    else if(element->type == ElementType_Int){
        print("%i", element->i);
    }
    else if(element->type == ElementType_Float){
        print("%f", element->f);
    }
    else if(element->type == ElementType_Array){
        print("[\n\t");
        print_element(element->sub_elements);
        print("\n]");
    }
    else if(element->type == ElementType_Object){
        print("{");
        brace_count++;
        print_element(element->sub_elements);
        brace_count--;
        //for(u32 i=0; i < brace_count; ++i){
        //    print("\t");
        //}
        print("}");
        //print("\n");
    }

    if(element->next_element){
        print(", ");
        if(element->type == ElementType_Array || element->type == ElementType_Object){
            print("\n\t");
        }
        print_element(element->next_element);
    }
}

static u32 found = 0;
static Pair points = {0};
static void traverse_element(JsonElement* element){
    do_next_element:;
    if(element->type == ElementType_Float){
        if(found == 0){
            points.x0 = element->f;
        }
        if(found == 1){
            points.y0 = element->f;
        }
        if(found == 2){
            points.x1 = element->f;
        }
        if(found == 3){
            points.y1 = element->f;
        }
        found++;
        if(found == 4){
            found = 0;
            Pair* p = push_struct(pm, Pair);
            *p = points;
        }
    }
    else if(element->type == ElementType_Array){
        traverse_element(element->sub_elements);
    }
    else if(element->type == ElementType_Object){
        traverse_element(element->sub_elements);
    }

    element = element->next_element;
    if(element){
        goto do_next_element;
    }
}

s32 main(s32 argc, char** argv){

    u64 seed = 0;
    u64 count = 0;
    if(argc == 3){
        seed = (u64)atoll(argv[1]);
        count = (u64)atoll(argv[2]);
    }
    else{
        print("Missing inputs [seed, count]\n");
        return(1);
    }

    init_clock(&clock);
    print("%llu\n", clock.frequency);

    //init seed
    s64 ticks = clock.get_ticks();
    random_seed((u64)ticks, 0);

    String8 cwd = os_get_cwd(tm);
    String8 filename = str8_literal("data.json");

    write_json_data(cwd, filename, count);
    JsonElement* result = parse_json(tm, cwd, filename);
    //print_element(result);
    traverse_element(result);

    print("\n");
    f64 sum = 0;
    f64 sum_coef = 1/(f64)count;
    Pair* at = (Pair*)pm->base;
    Pair* end = (Pair*)((u8*)pm->base + pm->at);
    while(at != end){
        //print("x0: %f - y0: %f - x1: %f - y1: %f\n", at->x0, at->y0, at->x1, at->y1);
        f64 earth_radius = 6372.8;
        f64 dist = reference_haversine(at, earth_radius);
        sum += (sum_coef * dist);
        at++;
    }

    print("Haversine sum: %f\n", sum);

    return(0);
}
