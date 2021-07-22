# cjson str

Simple JSON string builder written in C

https://tools.ietf.org/html/rfc8259

## example

```c
json_str_t *str = new_json_str(224);
json_str_begin_object(str, NULL);
  json_str_begin_object(str, "links");
    json_str_add_string(str, "self", "http://example.com/articles");
  json_str_end_object(str);
  json_str_begin_array(str, "data");
    json_str_begin_object(str, NULL);
      json_str_add_string(str, "type", "articles");
      json_str_add_integer(str, "id", 1);
      json_str_begin_object(str, "attributes");
        json_str_add_string(str, "title", "JSON:API paints my bikeshed!");
      json_str_end_object(str);
    json_str_end_object(str);

    json_str_begin_object(str, NULL);
      json_str_add_string(str, "type", "articles");
      json_str_add_integer(str, "id", 2);
      json_str_begin_object(str, "attributes");
        json_str_add_string(str, "title", "Rails is Omakase");
      json_str_end_object(str);
    json_str_end_object(str);
  json_str_end_array(str);
json_str_end_object(str);
puts(json_str_finalize(str));
delete_json_str(str);
```

above will generate following JSON from [json api example][https://jsonapi.org/format/#fetching-resources-responses]
```json
{"links":{"self":"http://example.com/articles"},"data":[{"type":"articles","id":1,"attributes":{"title":"JSON:API paints my bikeshed!"}},{"type":"articles","id":2,"attributes":{"title":"Rails is Omakase"}}]}
```

## API
