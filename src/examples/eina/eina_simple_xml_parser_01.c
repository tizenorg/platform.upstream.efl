//Compile with:
//gcc -Wall -o eina_simple_xml_parser_01 eina_simple_xml_parser_01.c `pkg-config --cflags --libs eina`

#include <Eina.h>
#include <stdio.h>
#include <string.h>

static Eina_Bool _xml_attr_cb(void *data, const char *key, const char *value);
static Eina_Bool _xml_tag_cb(void *data, Eina_Simple_XML_Type type,
		const char *content, unsigned offset, unsigned length);
static Eina_Bool _print(const void *container, void *data, void *fdata);

Eina_Bool tag_login   = EINA_FALSE;
Eina_Bool tag_message = EINA_FALSE;

int
main(void)
{
   FILE *file;
   long size;
   char *buffer;
   Eina_Array *array;

   eina_init();

   if ((file = fopen("chat.xml", "rb")))
     {
        fseek(file, 0, SEEK_END);
        size = ftell(file);
        fseek(file, 0, SEEK_SET);

        if ((buffer = malloc(size)))
          {
             fread(buffer, 1, size, file);

             array = eina_array_new(10);
             eina_simple_xml_parse(buffer, size, EINA_TRUE,
                                   _xml_tag_cb, array);

             eina_array_foreach(array, _print, NULL);
        
             eina_array_free(array);
             free(buffer);
          }
        else
          {
             EINA_LOG_ERR("Can't allocate memory!");
          }
        fclose(file);
     }
   else
     {
        EINA_LOG_ERR("Can't open chat.xml!");
     }
   eina_shutdown();

   return 0;
}

static Eina_Bool
_xml_tag_cb(void *data, Eina_Simple_XML_Type type, const char *content,
            unsigned offset, unsigned length)
{
   char buffer[length+1];
   Eina_Array *array = data;
   char str[512];

   if (type == EINA_SIMPLE_XML_OPEN)
     {
        if(!strncmp("post", content, strlen("post")))
          {
             const char *tags = eina_simple_xml_tag_attributes_find(content,
                                                                    length);
             eina_simple_xml_attributes_parse(tags, length - (tags - content),
                                              _xml_attr_cb, str);
          }
        else if (!strncmp("login>", content, strlen("login>")))
          {
             tag_login = EINA_TRUE;
          }
        else if (!strncmp("message>", content, strlen("message>")))
          {
             tag_message = EINA_TRUE;
          }
     }
   else if (type == EINA_SIMPLE_XML_DATA)
     {
        if (tag_login == EINA_TRUE)
          {
             snprintf(buffer, sizeof(buffer), content);
             strncat(str, "<", 1);
             strncat(str, buffer, sizeof(buffer));
             strncat(str, "> ", 2);
             tag_login = EINA_FALSE;
          }
        else if (tag_message == EINA_TRUE)
          {
             snprintf(buffer, sizeof(buffer), content);
             strncat(str, buffer, sizeof(buffer));
             tag_message = EINA_FALSE;
             eina_array_push(array, strdup(str));
          }
     }

   return EINA_TRUE;
}

static Eina_Bool
_xml_attr_cb(void *data, const char *key, const char *value)
{
   char *str = data;

   if(!strcmp("id", key))
   {
      snprintf(str, sizeof(value) + 3, "(%s) ", value);
   }

   return EINA_TRUE;
}

static Eina_Bool
_print(const void *container, void *data, void *fdata)
{
   printf("%s\n", (char *)data);

   return EINA_TRUE;
}
