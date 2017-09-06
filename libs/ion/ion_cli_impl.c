#include "ion_cli_impl.h"
#include "../muon/muon.h"
#include "interpreter.h"
#include "../muon/cli/neutron_cli_impl.h"

const char *ion_key = "ion";
const char *event_key = "events";
const char *ion_name = "ion";
extern string_t get_full_path(memid_t key);

// events are stored in the key called events.
result_t ion_action(cli_t *context)
  {
  result_t result;
  memid_t key;

  if(failed(result = reg_open_key(0, ion_key, &key)))
    {
    if(result != e_path_not_found)
      return result;

    if(failed(result = reg_create_key(0, ion_key, &key)))
      return result;

    memid_t events;
    // create the events key
    if(failed(result = reg_create_key(key, event_key, &events)))
      {
      // remove the new key
      reg_delete_key(key);
      return result;
      }
    }

  return cli_submode_enter(context, key, string_printf("%s ", ion_name));
  }

result_t ion_exit_action(cli_t *context)
  {
  cli_submode_exit(context);

  return s_ok;
  }

result_t ion_ls_name_action(cli_t *context, string_t path)
  {
  // see if a name given

  result_t result;
  memid_t child = 0;
  field_datatype dt = 0;
  char name[REG_NAME_MAX + 1];
  char buffer[32];  

  if (path != 0 && strcmp(path, event_key)== 0)
    {
    memid_t key;

    if (failed(result = reg_open_key(get_context(context), event_key, &key)))
        return result;

    // enumerate the keys
    dt = field_key;
    char name[REG_NAME_MAX + 1];
    char event_fn[REG_STRING_MAX + 1];

    memid_t child = 0;

    while (succeeded(reg_enum_key(key, &dt, 0, 0, REG_NAME_MAX, name, &child)))
      {
      uint16_t can_id = strtoul(name, 0, 10);

      // see if it is an enum value
      const enum_t *enum_value;
      if (succeeded(find_enum_name(can_ids, can_id, &enum_value)))
        {
        stream_puts(context->cfg.console_out, enum_value->name);
        stream_puts(context->cfg.console_out, "\r\n");
        }
      else
        stream_printf(context->cfg.console_out, "%d\r\n", can_id);

      // now enumerate the strings in it
      memid_t handler = 0;
      dt = field_string;

      while (succeeded(reg_enum_key(child, &dt, 0, 0, REG_NAME_MAX, name, &handler)))
        {
        string_t str;
        if (succeeded(reg_get_string(child, name, event_fn, 0)))
          stream_printf(context->cfg.console_out, "  %s : function %s(msg);\r\n", name, event_fn);

        dt = field_string;
        }

      dt = field_key;
      }
    }

  dt = field_stream;
  while(succeeded(result = reg_enum_key(get_context(context), &dt, 0, 0, REG_NAME_MAX, name, &child)))
    {
    name[REG_NAME_MAX] = 0;

    bool matched = true;
    // see if there is a wildcard match
    if(path != 0)
      {
      uint16_t len = string_length(path);
      uint16_t i;
      uint16_t p;
      for(i = 0, p=0; p < len && i < REG_NAME_MAX; i++)
        {
        if(path[p] == '?')
          {
          p++;
          continue;
          }

        if(path[p] == '*')
          break;

        if(name[i] == 0)
          {
          if(path[p] != 0 &&
             path[p] != '*')
            matched = false;

          break;
          }

        if(name[i] != path[p])
          {
          matched = false;
          break;
          }
        }
      }

    if (matched)
      {
      memid_t key;

      // we have to stat the file
      handle_t hndl;
      if (failed(stream_open(get_context(context), name, &hndl)))
        continue;

      uint16_t length;
      stream_length(hndl, &length);
      stream_close(hndl);

      // format as a 6 byte buffer
      uint16_t len = 0;
      uint16_t div = 10000;
      bool leading_0 = true;
      while (div > 0)
        {
        uint16_t n = length / div;
        length -= n * div;
        if (n > 0 || !leading_0 || div == 1)
          {
          buffer[len++] = '0' + n;
          leading_0 = false;
          }
        div /= 10;

        if (length == 0)
          break;
        }
      while (len < 6)
        buffer[len++] = ' ';

      buffer[6] = 0;
      strcpy(buffer + 6, name);
      strcat(buffer, "\r\n");

      stream_puts(context->cfg.console_out, buffer);
      }

    dt = field_stream;
    }

  return s_ok;
  }

result_t ion_rm_name_action(cli_t *context, string_t name)
  {
  result_t result;
  handle_t stream;

  // see if it exists
  if (failed(result = stream_open(get_context(context), name, &stream)))
    {
    if (result != e_path_not_found)
      return result;        // other error.

    stream_printf("Unknown stream %s\r\n", name);
    return result;
    }

  return stream_delete(stream);
  }

result_t ion_create_name_content_action(cli_t *context, string_t name, string_t content)
  {
  result_t result;
  handle_t stream;

  // see if it exists
  if (failed(result = stream_open(get_context(context), name, &stream)))
    {
    if (result != e_path_not_found)
      return result;        // other error.

    if (failed(result = stream_create(get_context(context), name, &stream)))
      return result;
    }

  if (content != 0)
    {
    result = stream_puts(stream, content);
    stream_close(stream);      
    }
  else
    {
    string_t full_path = string_ensure_size(string_create(0), 256);
    if (failed(result = stream_path(stream, true, 256, full_path)))
      {
      stream_close(stream);
      string_free(full_path);
      return result;
      }

    string_set_length(full_path, strlen(full_path));
    result = edit_script(context, full_path, stream);

    string_free(full_path);
    }

  return result;
  }

result_t ion_edit_name_action(cli_t *context, string_t name)
  {
  result_t result;
  handle_t stream;

  if(failed(result = stream_open(get_context(context), name, &stream)))
    return result;

  string_t full_path = string_ensure_size(string_create(0), 256);
  if (failed(result = stream_path(stream, true, 256, full_path)))
    {
    stream_close(stream);
    string_free(full_path);
    return result;
    }

  string_set_length(full_path, strlen(full_path));
  result = edit_script(context, full_path, stream);

  string_free(full_path);

  return result;
  }

result_t ion_cat_name_action(cli_t *context, string_t name)
  {
  result_t result;
  handle_t stream;

  if(failed(result = stream_open(get_context(context), name, &stream)))
    return result;

  stream_copy(stream, context->cfg.console_out);

  stream_close(stream);

  stream_puts(context->cfg.console_out, "\r\n");

  return s_ok;
  }

result_t ion_add_id_name_msg_handler_action(cli_t *context, uint16_t event_id, string_t ion_path, string_t event_handler)
  {
  memid_t key;
  result_t result;

  if (failed(result = reg_open_key(get_context(context), event_key, &key)))
    {
    if (result != e_path_not_found)
      return result;

    if (failed(result = reg_create_key(get_context(context), event_key, &key)))
      return result;
    }

  string_t key_name = string_printf("%d", event_id);

  // each event is a key, we add the string type and script path.
  memid_t event_key;
  if (failed(result = reg_open_key(key, key_name, &event_key)))
    {
    if (result != e_path_not_found ||
      failed(result = reg_create_key(key, key_name, &event_key)))
      {
      string_free(key_name);
      return result;
      }
    }

  string_free(key_name);

  // TODO: check the event handler exists in the parent directory

  // TODO: check the length of the strings....

  // we have an event handler, so let us set the prompts
  return reg_set_string(event_key, ion_path, event_handler);
  }

result_t ion_del_id_name_action(cli_t *context, uint16_t del_id, string_t ion_name)
  {
  result_t result;
  memid_t key;

  if (failed(result = reg_open_key(get_context(context), event_key, &key)))
    return result;

  string_t id_str = string_printf("%d", del_id);
  result = reg_open_key(get_context(context), id_str, &key);
  string_free(id_str);

  if (failed(result))
    return result;

  return reg_delete_value(key, ion_name);
  }

result_t ion_debug_name_action(cli_t *context, string_t path)
  {
  return e_not_implemented;
  }

result_t ion_exec_name_action(cli_t *context, string_t name)
  {
  result_t result;
  memid_t parent = 0;

  if (name != 0 &&
    failed(result = reg_open_key(0, ion_key, &parent)))
      return result;

  struct _ion_context_t *ion;
  if (failed(ion_create(parent, name, context->cfg.console_in, context->cfg.console_out,
    context->cfg.console_err, &ion)))
    {
    stream_printf(context->cfg.console_err, "Error when loading ion in interactive mode\r\n");
    return result;
    }

  if (failed(result = ion_exec(ion)))
    {
    stream_printf(context->cfg.console_err, "Error when loading running ion in interactive mode\r\n");
    }

  return ion_close(ion);
  }