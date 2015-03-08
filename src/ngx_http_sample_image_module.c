#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "parson/parson.h"
#include "http-get/http-get.h"

static char* ngx_http_sample_image(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_command_t ngx_http_sample_image_module_commands[] = {
  {
    ngx_string("ngx_sample_image"),
    NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
    ngx_http_sample_image,
    0,
    0,
    NULL,
  },
  ngx_null_command,
};

static ngx_http_module_t ngx_http_sample_image_module_ctx = {
  NULL,   /* preconfiguration */
  NULL,   /* postconfiguration */

  NULL,   /* create main configuration */
  NULL,   /* init main configuration */

  NULL,   /* create server configuration */
  NULL,   /* merge server configuration */

  NULL,   /* create location configuration */
  NULL,   /* merge location configuration */
};

ngx_module_t ngx_http_sample_image_module = {
  NGX_MODULE_V1,
  &ngx_http_sample_image_module_ctx,
  ngx_http_sample_image_module_commands,
  NGX_HTTP_MODULE,
  NULL,  /* init master */
  NULL,  /* init module */
  NULL,  /* init process */
  NULL,  /* init thread */
  NULL,  /* exit thread */
  NULL,  /* exit process */
  NULL,  /* exit master */
  NGX_MODULE_V1_PADDING
};

static char base_url[] = "https://www.googleapis.com/customsearch/v1?key=AIzaSyAKIZpQWd2tKt8gIvg8Pf3uWyvb7xV5MP8&cx=010886264418946358158:wvth-d_hsxs&searchType=image&q=";


static ngx_int_t ngx_http_sample_image_handler(ngx_http_request_t* r)
{
  ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, (const char*)r->uri.data);

  if (! (r->method & (NGX_HTTP_GET|NGX_HTTP_HEAD))) {
    return NGX_HTTP_NOT_ALLOWED;
  }

  ngx_http_complex_value_t cv;
  ngx_memzero(&cv, sizeof(ngx_http_complex_value_t));

  strtok((char*)(r->uri.data), "/");
  char *word = strtok(NULL, "/ ");

  ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "search word:%s", word);

  char search_url[512];
  char* json = NULL;
  ngx_memzero(search_url, 512);

  sprintf(search_url, "%s%s", base_url, word);
  ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "search url:%s", search_url);

  http_get_response_t* res = http_get(search_url);
  if (res->status != 200) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "search failed:%s", word);
    goto end;
  }

  json = calloc(res->size + 1, 1);
  if (!json) goto end;
  ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "jsonsize:%d", res->size);
  strncpy(json, res->data, res->size);

  JSON_Value *root_value = json_parse_string(json);
  JSON_Object *root = json_value_get_object(root_value);
  JSON_Array *items = json_object_dotget_array(root, "items");
  JSON_Object *item = json_array_get_object(items, 0);
  const char* link = json_object_dotget_string(item, "link");

  ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "link:%s", link);

  if (res) http_get_free(res);
  // get image
  res = http_get(link);
  if (res->status != 200) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "get failed:\n%s", link);
    goto end;
  }

  ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "image:\n%s", res->data);

  json_value_free(root_value);

end:

  if (json) free(json);
  //if (res) http_get_free(res);

  ngx_str_t jpeg_type = ngx_string("image/jpeg");
  cv.value.len = res->size;
  cv.value.data = (u_char*)(res->data);

  return ngx_http_send_response(r, NGX_HTTP_OK, &jpeg_type, &cv);
}



static char* ngx_http_sample_image(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
  ngx_http_core_loc_conf_t *clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
  clcf->handler = ngx_http_sample_image_handler;

  return NGX_CONF_OK;
}
