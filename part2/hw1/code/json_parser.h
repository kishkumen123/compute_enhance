#pragma once


enum TokenTypes{
    TokenType_Error,

    TokenType_Open_Brace,
    TokenType_Open_Bracket,
    TokenType_Close_Brace,
    TokenType_Close_Bracket,
    TokenType_Comma,
    TokenType_Colon,
    TokenType_Semi_Colon,

    TokenType_String_Literal,
    TokenType_Number,
    TokenType_True,
    TokenType_False,
    TokenType_Null,

    TokenType_Count,
};

struct JsonToken{
    TokenTypes type;
    String8 value;
};


static JsonToken
get_next_token(String8* data){
    JsonToken token = {};
    str8_eat_whitespace(data);
    if(data->size){

        token.type = TokenType_Error;
        token.value.str = data->str;
        token.value.size = 1;
        u8 value = *data->str;
        u8* start = data->str;
        str8_advance(data, 1);

        switch(value){
            case '{' :{ token.type = TokenType_Open_Brace; } break;
            case '}' :{ token.type = TokenType_Close_Brace; } break;
            case '[' :{ token.type = TokenType_Open_Bracket; } break;
            case ']' :{ token.type = TokenType_Close_Bracket; } break;
            case ',' :{ token.type = TokenType_Comma; } break;
            case ':' :{ token.type = TokenType_Colon; } break;
            case ';' :{ token.type = TokenType_Semi_Colon; } break;

            case '"':{
                token.type = TokenType_String_Literal;
                u8 char_at = *data->str;

                while(char_at != '"' && data->size){
                    if(char_at == '\\' && data->size){
                        if(*(data->str + 1) == '"'){
                            str8_advance(data, 1);
                            char_at = *data->str;
                        }
                    }
                    str8_advance(data, 1);
                    char_at = *data->str;
                }

                str8_advance(data, 1);
            } break;
            case 't':{
                if(str8(start, 4) == str8_literal("true")){
                    token.type = TokenType_True;
                    str8_advance(data, 3);
                    token.value.size = 4;
                }
            } break;
            case 'f':{
                if(str8(start, 5) == str8_literal("false")){
                    token.type = TokenType_False;
                    str8_advance(data, 4);
                    token.value.size = 5;
                }
            } break;
            case 'n':{
                if(str8(start, 4) == str8_literal("null")){
                    token.type = TokenType_Null;
                    str8_advance(data, 3);
                    token.value.size = 4;
                }
            } break;
            case '-':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':{
                token.type = TokenType_Number;

                u8 char_at = *data->str;
                if(char_at == '-' && data->size){
                    str8_advance(data, 1);
                    char_at = *data->str;
                }

                while(str8_is_digit(*data->str)){
                    str8_advance(data, 1);
                }
                char_at = *data->str;
                //if(value != '0' && data->size){
                //    while(str8_is_digit(*data->str)){
                //        str8_advance(data, 1);
                //    }
                //    char_at = *data->str;
                //}
                //else if(value == '0' && data->size){
                //    str8_advance(data, 1);
                //    char_at = *data->str;
                //}

                if(char_at == '.' && data->size){
                    str8_advance(data, 1);
                    while(str8_is_digit(*data->str)){
                        str8_advance(data, 1);
                    }
                    char_at = *data->str;
                }

                if((char_at == 'e' || char_at == 'E') && data->size){
                    str8_advance(data, 1);
                    char_at = *data->str;

                    if((char_at == '-' || char_at == '+') && data->size){
                        str8_advance(data, 1);
                        char_at = *data->str;
                    }

                    while(str8_is_digit(*data->str)){
                        str8_advance(data, 1);
                    }
                    char_at = *data->str;
                }

            } break;

        }
        token.value.size = (u64)(data->str - start);
    }

    return(token);
}
enum ElementType{
    ElementType_None,
    ElementType_Array,
    ElementType_Object,
    ElementType_Int,
    ElementType_Float,
    ElementType_Literal,
    ElementType_Bool,
    ElementType_Null,
};

struct JsonElement{
    JsonElement* next_element;
    JsonElement* sub_elements;
    ElementType type;
    u64 i;
    f64 f;
    bool b;
    String8 lit;
    String8 key;
};

static JsonElement* parse_element(Arena* arena, String8* data, String8 key, JsonToken token);
static JsonElement* parse_list(Arena* arena, String8* data, bool is_object){

    JsonElement* first_element = {0};
    JsonElement* last_element = {0};

    while(data->size){
        String8 key = {0};
        JsonToken token = get_next_token(data);
        if(is_object){
            if(token.type != TokenType_String_Literal){
                print("Error: Expected string literal (object key) after open brace\n");
                return(first_element);
            }
            key = token.value;

            JsonToken colon = get_next_token(data);
            if(colon.type != TokenType_Colon){
                print("Error: Expected colon after string literal (object key)\n");
                return(first_element);
            }
            token = get_next_token(data);
        }

        JsonElement* element = parse_element(arena, data, key, token);
        if(element){
            if(!first_element){
                first_element = element;
                last_element = element;
            }
            else{
                last_element->next_element = element;
                last_element = element;
            }
        }

        JsonToken separator = get_next_token(data);
        if(separator.type == TokenType_Close_Bracket || separator.type == TokenType_Close_Brace){
            break;
        }

    }
    return(first_element);
}

static JsonElement*
parse_element(Arena* arena, String8* data, String8 key, JsonToken token){

    JsonElement* sub_elements = {};
    JsonElement* element = push_struct(arena, JsonElement);

    if(key.size){
        element->key = key;
    }
    switch(token.type){
        case TokenType_Open_Bracket:{
            sub_elements = parse_list(arena, data, false);
            element->sub_elements = sub_elements;
            element->type = ElementType_Array;
        } break;
        case TokenType_Open_Brace:{
            sub_elements = parse_list(arena, data, true);
            element->sub_elements = sub_elements;
            element->type = ElementType_Object;
        } break;
        case TokenType_Null:{
            element->type = ElementType_Null;
            element->i = 0;
        } break;
        case TokenType_True:{
            element->type = ElementType_Bool;
            element->b = true;
        } break;
        case TokenType_False:{
            element->type = ElementType_Bool;
            element->b = false;
        } break;
        case TokenType_String_Literal:{
            element->type = ElementType_Literal;
            element->lit = token.value;
        } break;
        case TokenType_Number:{
            ScratchArena scratch = begin_scratch(0);
            String8 null_value = str8_null_terminate(scratch.arena, token.value);

            char* end;
            u64 int_result = strtoull((char*)null_value.str, &end, 10);
            if(*end == 0){
                element->type = ElementType_Int;
                element->i = int_result;
            }
            else{
                f64 float_result = strtod((char*)null_value.str, &end);
                element->type = ElementType_Float;
                element->f = float_result;
            }
            end_scratch(scratch);
        } break;
    }

    return(element);
}

static JsonElement*
parse_json(Arena* arena, String8 dir, String8 filename){
    File file = os_file_open(dir, filename);
    assert_fh(file);

    String8 data = os_file_read(arena, &file);
    data.size--; // dont try and parse the final null terminator in the file

    JsonToken token = get_next_token(&data);
    JsonElement* result = parse_element(arena, &data, {}, token);
    return(result);
}

