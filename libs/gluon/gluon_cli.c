#include "gluon_cli.h"
/* This file is generated by cli */

static result_t cli_gluon_ls_name (cli_t *parser)
  {
  result_t result;
  const char * gluon_ls_name_ = 0;

  if(parser->tokens[1].token_length > 0 &&
    failed(result = cli_get_string(&parser->tokens[1], &gluon_ls_name_)))
    return result;

  result = gluon_ls_name_action(parser, ((parser->tokens[1].token_length > 0) ? gluon_ls_name_ : 0));


  return result;
  }


static result_t cli_gluon_rm_name (cli_t *parser)
  {
  result_t result;
  const char * gluon_rm_name_ = 0;

  if(failed(result = cli_get_string(&parser->tokens[1], &gluon_rm_name_)))
    return result;

  result = gluon_rm_name_action(parser, gluon_rm_name_);


  return result;
  }


static result_t cli_gluon_show_name_from_to (cli_t *parser)
  {
  result_t result;
  const char * gluon_show_name_ = 0;
  const char * gluon_show_name_from_ = 0;
  const char * gluon_show_name_from_to_ = 0;

  if(failed(result = cli_get_string(&parser->tokens[1], &gluon_show_name_)))
    return result;

  if(parser->tokens[2].token_length > 0 &&
    failed(result = cli_get_string(&parser->tokens[2], &gluon_show_name_from_)))
    return result;

  if(parser->tokens[3].token_length > 0 &&
    failed(result = cli_get_string(&parser->tokens[3], &gluon_show_name_from_to_)))
    return result;

  result = gluon_show_name_from_to_action(parser, gluon_show_name_, ((parser->tokens[2].token_length > 0) ? gluon_show_name_from_ : 0), ((parser->tokens[3].token_length > 0) ? gluon_show_name_from_to_ : 0));


  return result;
  }


static result_t cli_gluon_query_name (cli_t *parser)
  {
  result_t result;
  const char * gluon_query_name_ = 0;

  if(failed(result = cli_get_string(&parser->tokens[1], &gluon_query_name_)))
    return result;

  result = gluon_query_name_action(parser, gluon_query_name_);


  return result;
  }


static result_t cli_gluon_capture_name_rate_rate (cli_t *parser)
  {
  result_t result;
  uint16_t gluon_capture_name_rate_rate_;

  if(failed(result = cli_get_uint16(&parser->tokens[1], &gluon_capture_name_rate_rate_)))
    return result;

  result = gluon_capture_name_rate_rate_action(parser, gluon_capture_name_rate_rate_);


  return result;
  }


static result_t cli_gluon_capture_name_trigger_eq_id_value (cli_t *parser)
  {
  result_t result;
  uint16_t gluon_capture_name_trigger_eq_id_;
  int16_t gluon_capture_name_trigger_eq_id_value_;

  if(failed(result = cli_get_enum(&parser->tokens[1], can_ids, &gluon_capture_name_trigger_eq_id_)))
    return result;

  if(failed(result = cli_get_int16(&parser->tokens[2], &gluon_capture_name_trigger_eq_id_value_)))
    return result;

  result = gluon_capture_name_trigger_eq_id_value_action(parser, gluon_capture_name_trigger_eq_id_, gluon_capture_name_trigger_eq_id_value_);


  return result;
  }


static result_t cli_gluon_capture_name_trigger_lt_id_value (cli_t *parser)
  {
  result_t result;
  uint16_t gluon_capture_name_trigger_lt_id_;
  int16_t gluon_capture_name_trigger_lt_id_value_;

  if(failed(result = cli_get_enum(&parser->tokens[1], can_ids, &gluon_capture_name_trigger_lt_id_)))
    return result;

  if(failed(result = cli_get_int16(&parser->tokens[2], &gluon_capture_name_trigger_lt_id_value_)))
    return result;

  result = gluon_capture_name_trigger_lt_id_value_action(parser, gluon_capture_name_trigger_lt_id_, gluon_capture_name_trigger_lt_id_value_);


  return result;
  }


static result_t cli_gluon_capture_name_trigger_gt_id_value (cli_t *parser)
  {
  result_t result;
  uint16_t gluon_capture_name_trigger_gt_id_;
  int16_t gluon_capture_name_trigger_gt_id_value_;

  if(failed(result = cli_get_enum(&parser->tokens[1], can_ids, &gluon_capture_name_trigger_gt_id_)))
    return result;

  if(failed(result = cli_get_int16(&parser->tokens[2], &gluon_capture_name_trigger_gt_id_value_)))
    return result;

  result = gluon_capture_name_trigger_gt_id_value_action(parser, gluon_capture_name_trigger_gt_id_, gluon_capture_name_trigger_gt_id_value_);


  return result;
  }


static result_t cli_gluon_capture_name_trigger_eqf_id_value (cli_t *parser)
  {
  result_t result;
  uint16_t gluon_capture_name_trigger_eqf_id_;
  float gluon_capture_name_trigger_eqf_id_value_;

  if(failed(result = cli_get_enum(&parser->tokens[1], can_ids, &gluon_capture_name_trigger_eqf_id_)))
    return result;

  if(failed(result = cli_get_float(&parser->tokens[2], &gluon_capture_name_trigger_eqf_id_value_)))
    return result;

  result = gluon_capture_name_trigger_eqf_id_value_action(parser, gluon_capture_name_trigger_eqf_id_, gluon_capture_name_trigger_eqf_id_value_);


  return result;
  }


static result_t cli_gluon_capture_name_trigger_ltf_id_value (cli_t *parser)
  {
  result_t result;
  uint16_t gluon_capture_name_trigger_ltf_id_;
  float gluon_capture_name_trigger_ltf_id_value_;

  if(failed(result = cli_get_enum(&parser->tokens[1], can_ids, &gluon_capture_name_trigger_ltf_id_)))
    return result;

  if(failed(result = cli_get_float(&parser->tokens[2], &gluon_capture_name_trigger_ltf_id_value_)))
    return result;

  result = gluon_capture_name_trigger_ltf_id_value_action(parser, gluon_capture_name_trigger_ltf_id_, gluon_capture_name_trigger_ltf_id_value_);


  return result;
  }


static result_t cli_gluon_capture_name_trigger_gtf_id_value (cli_t *parser)
  {
  result_t result;
  uint16_t gluon_capture_name_trigger_gtf_id_;
  float gluon_capture_name_trigger_gtf_id_value_;

  if(failed(result = cli_get_enum(&parser->tokens[1], can_ids, &gluon_capture_name_trigger_gtf_id_)))
    return result;

  if(failed(result = cli_get_float(&parser->tokens[2], &gluon_capture_name_trigger_gtf_id_value_)))
    return result;

  result = gluon_capture_name_trigger_gtf_id_value_action(parser, gluon_capture_name_trigger_gtf_id_, gluon_capture_name_trigger_gtf_id_value_);


  return result;
  }


static result_t cli_gluon_capture_name_trigger_rm_eq_id (cli_t *parser)
  {
  result_t result;
  uint16_t gluon_capture_name_trigger_rm_eq_id_;

  if(failed(result = cli_get_enum(&parser->tokens[2], can_ids, &gluon_capture_name_trigger_rm_eq_id_)))
    return result;

  result = gluon_capture_name_trigger_rm_eq_id_action(parser, gluon_capture_name_trigger_rm_eq_id_);


  return result;
  }


static result_t cli_gluon_capture_name_trigger_rm_lt_id (cli_t *parser)
  {
  result_t result;
  uint16_t gluon_capture_name_trigger_rm_lt_id_;

  if(failed(result = cli_get_enum(&parser->tokens[2], can_ids, &gluon_capture_name_trigger_rm_lt_id_)))
    return result;

  result = gluon_capture_name_trigger_rm_lt_id_action(parser, gluon_capture_name_trigger_rm_lt_id_);


  return result;
  }


static result_t cli_gluon_capture_name_trigger_rm_gt_id (cli_t *parser)
  {
  result_t result;
  uint16_t gluon_capture_name_trigger_rm_gt_id_;

  if(failed(result = cli_get_enum(&parser->tokens[2], can_ids, &gluon_capture_name_trigger_rm_gt_id_)))
    return result;

  result = gluon_capture_name_trigger_rm_gt_id_action(parser, gluon_capture_name_trigger_rm_gt_id_);


  return result;
  }


static result_t cli_gluon_capture_name_trigger_rm_eqf_id (cli_t *parser)
  {
  result_t result;
  uint16_t gluon_capture_name_trigger_rm_eqf_id_;

  if(failed(result = cli_get_enum(&parser->tokens[2], can_ids, &gluon_capture_name_trigger_rm_eqf_id_)))
    return result;

  result = gluon_capture_name_trigger_rm_eqf_id_action(parser, gluon_capture_name_trigger_rm_eqf_id_);


  return result;
  }


static result_t cli_gluon_capture_name_trigger_rm_ltf_id (cli_t *parser)
  {
  result_t result;
  uint16_t gluon_capture_name_trigger_rm_ltf_id_;

  if(failed(result = cli_get_enum(&parser->tokens[2], can_ids, &gluon_capture_name_trigger_rm_ltf_id_)))
    return result;

  result = gluon_capture_name_trigger_rm_ltf_id_action(parser, gluon_capture_name_trigger_rm_ltf_id_);


  return result;
  }


static result_t cli_gluon_capture_name_trigger_rm_gtf_id (cli_t *parser)
  {
  result_t result;
  uint16_t gluon_capture_name_trigger_rm_gtf_id_;

  if(failed(result = cli_get_enum(&parser->tokens[2], can_ids, &gluon_capture_name_trigger_rm_gtf_id_)))
    return result;

  result = gluon_capture_name_trigger_rm_gtf_id_action(parser, gluon_capture_name_trigger_rm_gtf_id_);


  return result;
  }


static result_t cli_gluon_capture_name_trigger_exit (cli_t *parser)
  {
  result_t result;
  result = gluon_capture_name_trigger_exit_action(parser);


  return result;
  }


static result_t cli_gluon_capture_name_trigger (cli_t *parser)
  {
  result_t result;
  result = gluon_capture_name_trigger_action(parser);


  return result;
  }


static result_t cli_gluon_capture_name_ls (cli_t *parser)
  {
  result_t result;
  result = gluon_capture_name_ls_action(parser);


  return result;
  }


static result_t cli_gluon_capture_name_id_id_name (cli_t *parser)
  {
  result_t result;
  uint16_t gluon_capture_name_id_id_;
  const char * gluon_capture_name_id_id_name_ = 0;

  if(failed(result = cli_get_enum(&parser->tokens[1], can_ids, &gluon_capture_name_id_id_)))
    return result;

  if(parser->tokens[2].token_length > 0 &&
    failed(result = cli_get_string(&parser->tokens[2], &gluon_capture_name_id_id_name_)))
    return result;

  result = gluon_capture_name_id_id_name_action(parser, gluon_capture_name_id_id_, ((parser->tokens[2].token_length > 0) ? gluon_capture_name_id_id_name_ : 0));


  return result;
  }


static result_t cli_gluon_capture_name_rm_id (cli_t *parser)
  {
  result_t result;
  uint16_t gluon_capture_name_rm_id_;

  if(failed(result = cli_get_enum(&parser->tokens[1], can_ids, &gluon_capture_name_rm_id_)))
    return result;

  result = gluon_capture_name_rm_id_action(parser, gluon_capture_name_rm_id_);


  return result;
  }


static result_t cli_gluon_capture_name_exit (cli_t *parser)
  {
  result_t result;
  result = gluon_capture_name_exit_action(parser);


  return result;
  }


static result_t cli_gluon_capture_name (cli_t *parser)
  {
  result_t result;
  const char * gluon_capture_name_ = 0;

  if(failed(result = cli_get_string(&parser->tokens[1], &gluon_capture_name_)))
    return result;

  result = gluon_capture_name_action(parser, gluon_capture_name_);


  return result;
  }


static result_t cli_gluon_exit (cli_t *parser)
  {
  result_t result;
  result = gluon_exit_action(parser);


  return result;
  }


static result_t cli_gluon (cli_t *parser)
  {
  result_t result;
  result = gluon_action(parser);


  return result;
  }


static cli_node_t node_gluon_ls_name_end;

static cli_node_t node_gluon_ls_name;

static cli_node_t node_gluon_ls;

static cli_node_t node_gluon_rm_name_end;

static cli_node_t node_gluon_rm_name;

static cli_node_t node_gluon_rm;

static cli_node_t node_gluon_show_name_from_to_end;

static cli_node_t node_gluon_show_name_from_to;

static cli_node_t node_gluon_show_name_from;

static cli_node_t node_gluon_show_name;

static cli_node_t node_gluon_show;

static cli_node_t node_gluon_query_name_end;

static cli_node_t node_gluon_query_name;

static cli_node_t node_gluon_query;

static cli_node_t node_gluon_capture_name_rate_rate_end;

static cli_node_t node_gluon_capture_name_rate_rate;

static cli_node_t node_gluon_capture_name_rate;

static cli_node_t node_gluon_capture_name_trigger_eq_id_value_end;

static cli_node_t node_gluon_capture_name_trigger_eq_id_value;

static cli_node_t node_gluon_capture_name_trigger_eq_id;

static cli_node_t node_gluon_capture_name_trigger_eq;

static cli_node_t node_gluon_capture_name_trigger_lt_id_value_end;

static cli_node_t node_gluon_capture_name_trigger_lt_id_value;

static cli_node_t node_gluon_capture_name_trigger_lt_id;

static cli_node_t node_gluon_capture_name_trigger_lt;

static cli_node_t node_gluon_capture_name_trigger_gt_id_value_end;

static cli_node_t node_gluon_capture_name_trigger_gt_id_value;

static cli_node_t node_gluon_capture_name_trigger_gt_id;

static cli_node_t node_gluon_capture_name_trigger_gt;

static cli_node_t node_gluon_capture_name_trigger_eqf_id_value_end;

static cli_node_t node_gluon_capture_name_trigger_eqf_id_value;

static cli_node_t node_gluon_capture_name_trigger_eqf_id;

static cli_node_t node_gluon_capture_name_trigger_eqf;

static cli_node_t node_gluon_capture_name_trigger_ltf_id_value_end;

static cli_node_t node_gluon_capture_name_trigger_ltf_id_value;

static cli_node_t node_gluon_capture_name_trigger_ltf_id;

static cli_node_t node_gluon_capture_name_trigger_ltf;

static cli_node_t node_gluon_capture_name_trigger_gtf_id_value_end;

static cli_node_t node_gluon_capture_name_trigger_gtf_id_value;

static cli_node_t node_gluon_capture_name_trigger_gtf_id;

static cli_node_t node_gluon_capture_name_trigger_gtf;

static cli_node_t node_gluon_capture_name_trigger_rm_eq_id_end;

static cli_node_t node_gluon_capture_name_trigger_rm_eq_id;

static cli_node_t node_gluon_capture_name_trigger_rm_eq;

static cli_node_t node_gluon_capture_name_trigger_rm_lt_id_end;

static cli_node_t node_gluon_capture_name_trigger_rm_lt_id;

static cli_node_t node_gluon_capture_name_trigger_rm_lt;

static cli_node_t node_gluon_capture_name_trigger_rm_gt_id_end;

static cli_node_t node_gluon_capture_name_trigger_rm_gt_id;

static cli_node_t node_gluon_capture_name_trigger_rm_gt;

static cli_node_t node_gluon_capture_name_trigger_rm_eqf_id_end;

static cli_node_t node_gluon_capture_name_trigger_rm_eqf_id;

static cli_node_t node_gluon_capture_name_trigger_rm_eqf;

static cli_node_t node_gluon_capture_name_trigger_rm_ltf_id_end;

static cli_node_t node_gluon_capture_name_trigger_rm_ltf_id;

static cli_node_t node_gluon_capture_name_trigger_rm_ltf;

static cli_node_t node_gluon_capture_name_trigger_rm_gtf_id_end;

static cli_node_t node_gluon_capture_name_trigger_rm_gtf_id;

static cli_node_t node_gluon_capture_name_trigger_rm_gtf;

static cli_node_t node_gluon_capture_name_trigger_rm;

static cli_node_t node_gluon_capture_name_trigger_exit_end;

static cli_node_t node_gluon_capture_name_trigger_exit;

static cli_node_t node_gluon_capture_name_trigger_end;

static cli_node_t node_gluon_capture_name_trigger;

static cli_node_t node_gluon_capture_name_ls_end;

static cli_node_t node_gluon_capture_name_ls;

static cli_node_t node_gluon_capture_name_id_id_name_end;

static cli_node_t node_gluon_capture_name_id_id_name;

static cli_node_t node_gluon_capture_name_id_id;

static cli_node_t node_gluon_capture_name_id;

static cli_node_t node_gluon_capture_name_rm_id_end;

static cli_node_t node_gluon_capture_name_rm_id;

static cli_node_t node_gluon_capture_name_rm;

static cli_node_t node_gluon_capture_name_exit_end;

static cli_node_t node_gluon_capture_name_exit;

static cli_node_t node_gluon_capture_name_end;

static cli_node_t node_gluon_capture_name;

static cli_node_t node_gluon_capture;

static cli_node_t node_gluon_exit_end;

static cli_node_t node_gluon_exit;

static cli_node_t node_gluon_end;

static cli_node_t node_gluon;

static cli_node_t node_gluon_ls_name_end = {
  CLI_NODE_END,
  CLI_NODE_FLAGS_OPT_END,
  cli_gluon_ls_name,
  0,
  0,
  0
  };


static cli_node_t node_gluon_ls_name = {
  CLI_NODE_STRING,
  CLI_NODE_FLAGS_OPT_START | CLI_NODE_FLAGS_OPT_END | CLI_NODE_FLAGS_OPT_PARTIAL,
  "STRING:name",
  0,
  0,
  &node_gluon_ls_name_end
  };


static cli_node_t node_gluon_ls = {
  CLI_NODE_KEYWORD,
  0,
  "ls",
  0,
  &node_gluon_rm,
  &node_gluon_ls_name
  };


static cli_node_t node_gluon_rm_name_end = {
  CLI_NODE_END,
  CLI_NODE_FLAGS_OPT_END,
  cli_gluon_rm_name,
  0,
  0,
  0
  };


static cli_node_t node_gluon_rm_name = {
  CLI_NODE_STRING,
  0,
  "STRING:name",
  0,
  0,
  &node_gluon_rm_name_end
  };


static cli_node_t node_gluon_rm = {
  CLI_NODE_KEYWORD,
  0,
  "rm",
  0,
  &node_gluon_show,
  &node_gluon_rm_name
  };


static cli_node_t node_gluon_show_name_from_to_end = {
  CLI_NODE_END,
  CLI_NODE_FLAGS_OPT_END,
  cli_gluon_show_name_from_to,
  0,
  0,
  0
  };


static cli_node_t node_gluon_show_name_from_to = {
  CLI_NODE_STRING,
  CLI_NODE_FLAGS_OPT_START | CLI_NODE_FLAGS_OPT_END | CLI_NODE_FLAGS_OPT_PARTIAL,
  "STRING:to",
  0,
  0,
  &node_gluon_show_name_from_to_end
  };


static cli_node_t node_gluon_show_name_from = {
  CLI_NODE_STRING,
  CLI_NODE_FLAGS_OPT_START | CLI_NODE_FLAGS_OPT_PARTIAL,
  "STRING:from",
  0,
  0,
  &node_gluon_show_name_from_to
  };


static cli_node_t node_gluon_show_name = {
  CLI_NODE_STRING,
  0,
  "STRING:name",
  0,
  0,
  &node_gluon_show_name_from
  };


static cli_node_t node_gluon_show = {
  CLI_NODE_KEYWORD,
  0,
  "show",
  0,
  &node_gluon_query,
  &node_gluon_show_name
  };


static cli_node_t node_gluon_query_name_end = {
  CLI_NODE_END,
  CLI_NODE_FLAGS_OPT_END,
  cli_gluon_query_name,
  0,
  0,
  0
  };


static cli_node_t node_gluon_query_name = {
  CLI_NODE_STRING,
  0,
  "STRING:name",
  0,
  0,
  &node_gluon_query_name_end
  };


static cli_node_t node_gluon_query = {
  CLI_NODE_KEYWORD,
  0,
  "query",
  0,
  &node_gluon_capture,
  &node_gluon_query_name
  };


static cli_node_t node_gluon_capture_name_rate_rate_end = {
  CLI_NODE_END,
  CLI_NODE_FLAGS_OPT_END,
  cli_gluon_capture_name_rate_rate,
  0,
  0,
  0
  };


static cli_node_t node_gluon_capture_name_rate_rate = {
  CLI_NODE_UINT16,
  0,
  "UINT16:rate",
  0,
  0,
  &node_gluon_capture_name_rate_rate_end
  };


static cli_node_t node_gluon_capture_name_rate = {
  CLI_NODE_KEYWORD,
  0,
  "rate",
  0,
  &node_gluon_capture_name_trigger,
  &node_gluon_capture_name_rate_rate
  };


static cli_node_t node_gluon_capture_name_trigger_eq_id_value_end = {
  CLI_NODE_END,
  CLI_NODE_FLAGS_OPT_END,
  cli_gluon_capture_name_trigger_eq_id_value,
  0,
  0,
  0
  };


static cli_node_t node_gluon_capture_name_trigger_eq_id_value = {
  CLI_NODE_INT16,
  0,
  "INT16:value",
  0,
  0,
  &node_gluon_capture_name_trigger_eq_id_value_end
  };


static cli_node_t node_gluon_capture_name_trigger_eq_id = {
  CLI_NODE_ENUM,
  0,
  &can_ids,
  0,
  0,
  &node_gluon_capture_name_trigger_eq_id_value
  };


static cli_node_t node_gluon_capture_name_trigger_eq = {
  CLI_NODE_KEYWORD,
  0,
  "eq",
  0,
  &node_gluon_capture_name_trigger_lt,
  &node_gluon_capture_name_trigger_eq_id
  };


static cli_node_t node_gluon_capture_name_trigger_lt_id_value_end = {
  CLI_NODE_END,
  CLI_NODE_FLAGS_OPT_END,
  cli_gluon_capture_name_trigger_lt_id_value,
  0,
  0,
  0
  };


static cli_node_t node_gluon_capture_name_trigger_lt_id_value = {
  CLI_NODE_INT16,
  0,
  "INT16:value",
  0,
  0,
  &node_gluon_capture_name_trigger_lt_id_value_end
  };


static cli_node_t node_gluon_capture_name_trigger_lt_id = {
  CLI_NODE_ENUM,
  0,
  &can_ids,
  0,
  0,
  &node_gluon_capture_name_trigger_lt_id_value
  };


static cli_node_t node_gluon_capture_name_trigger_lt = {
  CLI_NODE_KEYWORD,
  0,
  "lt",
  0,
  &node_gluon_capture_name_trigger_gt,
  &node_gluon_capture_name_trigger_lt_id
  };


static cli_node_t node_gluon_capture_name_trigger_gt_id_value_end = {
  CLI_NODE_END,
  CLI_NODE_FLAGS_OPT_END,
  cli_gluon_capture_name_trigger_gt_id_value,
  0,
  0,
  0
  };


static cli_node_t node_gluon_capture_name_trigger_gt_id_value = {
  CLI_NODE_INT16,
  0,
  "INT16:value",
  0,
  0,
  &node_gluon_capture_name_trigger_gt_id_value_end
  };


static cli_node_t node_gluon_capture_name_trigger_gt_id = {
  CLI_NODE_ENUM,
  0,
  &can_ids,
  0,
  0,
  &node_gluon_capture_name_trigger_gt_id_value
  };


static cli_node_t node_gluon_capture_name_trigger_gt = {
  CLI_NODE_KEYWORD,
  0,
  "gt",
  0,
  &node_gluon_capture_name_trigger_eqf,
  &node_gluon_capture_name_trigger_gt_id
  };


static cli_node_t node_gluon_capture_name_trigger_eqf_id_value_end = {
  CLI_NODE_END,
  CLI_NODE_FLAGS_OPT_END,
  cli_gluon_capture_name_trigger_eqf_id_value,
  0,
  0,
  0
  };


static cli_node_t node_gluon_capture_name_trigger_eqf_id_value = {
  CLI_NODE_FLOAT,
  0,
  "FLOAT:value",
  0,
  0,
  &node_gluon_capture_name_trigger_eqf_id_value_end
  };


static cli_node_t node_gluon_capture_name_trigger_eqf_id = {
  CLI_NODE_ENUM,
  0,
  &can_ids,
  0,
  0,
  &node_gluon_capture_name_trigger_eqf_id_value
  };


static cli_node_t node_gluon_capture_name_trigger_eqf = {
  CLI_NODE_KEYWORD,
  0,
  "eqf",
  0,
  &node_gluon_capture_name_trigger_ltf,
  &node_gluon_capture_name_trigger_eqf_id
  };


static cli_node_t node_gluon_capture_name_trigger_ltf_id_value_end = {
  CLI_NODE_END,
  CLI_NODE_FLAGS_OPT_END,
  cli_gluon_capture_name_trigger_ltf_id_value,
  0,
  0,
  0
  };


static cli_node_t node_gluon_capture_name_trigger_ltf_id_value = {
  CLI_NODE_FLOAT,
  0,
  "FLOAT:value",
  0,
  0,
  &node_gluon_capture_name_trigger_ltf_id_value_end
  };


static cli_node_t node_gluon_capture_name_trigger_ltf_id = {
  CLI_NODE_ENUM,
  0,
  &can_ids,
  0,
  0,
  &node_gluon_capture_name_trigger_ltf_id_value
  };


static cli_node_t node_gluon_capture_name_trigger_ltf = {
  CLI_NODE_KEYWORD,
  0,
  "ltf",
  0,
  &node_gluon_capture_name_trigger_gtf,
  &node_gluon_capture_name_trigger_ltf_id
  };


static cli_node_t node_gluon_capture_name_trigger_gtf_id_value_end = {
  CLI_NODE_END,
  CLI_NODE_FLAGS_OPT_END,
  cli_gluon_capture_name_trigger_gtf_id_value,
  0,
  0,
  0
  };


static cli_node_t node_gluon_capture_name_trigger_gtf_id_value = {
  CLI_NODE_FLOAT,
  0,
  "FLOAT:value",
  0,
  0,
  &node_gluon_capture_name_trigger_gtf_id_value_end
  };


static cli_node_t node_gluon_capture_name_trigger_gtf_id = {
  CLI_NODE_ENUM,
  0,
  &can_ids,
  0,
  0,
  &node_gluon_capture_name_trigger_gtf_id_value
  };


static cli_node_t node_gluon_capture_name_trigger_gtf = {
  CLI_NODE_KEYWORD,
  0,
  "gtf",
  0,
  &node_gluon_capture_name_trigger_rm,
  &node_gluon_capture_name_trigger_gtf_id
  };


static cli_node_t node_gluon_capture_name_trigger_rm_eq_id_end = {
  CLI_NODE_END,
  CLI_NODE_FLAGS_OPT_END,
  cli_gluon_capture_name_trigger_rm_eq_id,
  0,
  0,
  0
  };


static cli_node_t node_gluon_capture_name_trigger_rm_eq_id = {
  CLI_NODE_ENUM,
  0,
  &can_ids,
  0,
  0,
  &node_gluon_capture_name_trigger_rm_eq_id_end
  };


static cli_node_t node_gluon_capture_name_trigger_rm_eq = {
  CLI_NODE_KEYWORD,
  0,
  "eq",
  0,
  &node_gluon_capture_name_trigger_rm_lt,
  &node_gluon_capture_name_trigger_rm_eq_id
  };


static cli_node_t node_gluon_capture_name_trigger_rm_lt_id_end = {
  CLI_NODE_END,
  CLI_NODE_FLAGS_OPT_END,
  cli_gluon_capture_name_trigger_rm_lt_id,
  0,
  0,
  0
  };


static cli_node_t node_gluon_capture_name_trigger_rm_lt_id = {
  CLI_NODE_ENUM,
  0,
  &can_ids,
  0,
  0,
  &node_gluon_capture_name_trigger_rm_lt_id_end
  };


static cli_node_t node_gluon_capture_name_trigger_rm_lt = {
  CLI_NODE_KEYWORD,
  0,
  "lt",
  0,
  &node_gluon_capture_name_trigger_rm_gt,
  &node_gluon_capture_name_trigger_rm_lt_id
  };


static cli_node_t node_gluon_capture_name_trigger_rm_gt_id_end = {
  CLI_NODE_END,
  CLI_NODE_FLAGS_OPT_END,
  cli_gluon_capture_name_trigger_rm_gt_id,
  0,
  0,
  0
  };


static cli_node_t node_gluon_capture_name_trigger_rm_gt_id = {
  CLI_NODE_ENUM,
  0,
  &can_ids,
  0,
  0,
  &node_gluon_capture_name_trigger_rm_gt_id_end
  };


static cli_node_t node_gluon_capture_name_trigger_rm_gt = {
  CLI_NODE_KEYWORD,
  0,
  "gt",
  0,
  &node_gluon_capture_name_trigger_rm_eqf,
  &node_gluon_capture_name_trigger_rm_gt_id
  };


static cli_node_t node_gluon_capture_name_trigger_rm_eqf_id_end = {
  CLI_NODE_END,
  CLI_NODE_FLAGS_OPT_END,
  cli_gluon_capture_name_trigger_rm_eqf_id,
  0,
  0,
  0
  };


static cli_node_t node_gluon_capture_name_trigger_rm_eqf_id = {
  CLI_NODE_ENUM,
  0,
  &can_ids,
  0,
  0,
  &node_gluon_capture_name_trigger_rm_eqf_id_end
  };


static cli_node_t node_gluon_capture_name_trigger_rm_eqf = {
  CLI_NODE_KEYWORD,
  0,
  "eqf",
  0,
  &node_gluon_capture_name_trigger_rm_ltf,
  &node_gluon_capture_name_trigger_rm_eqf_id
  };


static cli_node_t node_gluon_capture_name_trigger_rm_ltf_id_end = {
  CLI_NODE_END,
  CLI_NODE_FLAGS_OPT_END,
  cli_gluon_capture_name_trigger_rm_ltf_id,
  0,
  0,
  0
  };


static cli_node_t node_gluon_capture_name_trigger_rm_ltf_id = {
  CLI_NODE_ENUM,
  0,
  &can_ids,
  0,
  0,
  &node_gluon_capture_name_trigger_rm_ltf_id_end
  };


static cli_node_t node_gluon_capture_name_trigger_rm_ltf = {
  CLI_NODE_KEYWORD,
  0,
  "ltf",
  0,
  &node_gluon_capture_name_trigger_rm_gtf,
  &node_gluon_capture_name_trigger_rm_ltf_id
  };


static cli_node_t node_gluon_capture_name_trigger_rm_gtf_id_end = {
  CLI_NODE_END,
  CLI_NODE_FLAGS_OPT_END,
  cli_gluon_capture_name_trigger_rm_gtf_id,
  0,
  0,
  0
  };


static cli_node_t node_gluon_capture_name_trigger_rm_gtf_id = {
  CLI_NODE_ENUM,
  0,
  &can_ids,
  0,
  0,
  &node_gluon_capture_name_trigger_rm_gtf_id_end
  };


static cli_node_t node_gluon_capture_name_trigger_rm_gtf = {
  CLI_NODE_KEYWORD,
  0,
  "gtf",
  0,
  0,
  &node_gluon_capture_name_trigger_rm_gtf_id
  };


static cli_node_t node_gluon_capture_name_trigger_rm = {
  CLI_NODE_KEYWORD,
  0,
  "rm",
  0,
  &node_gluon_capture_name_trigger_exit,
  &node_gluon_capture_name_trigger_rm_eq
  };


static cli_node_t node_gluon_capture_name_trigger_exit_end = {
  CLI_NODE_END,
  CLI_NODE_FLAGS_OPT_END,
  cli_gluon_capture_name_trigger_exit,
  0,
  0,
  0
  };


static cli_node_t node_gluon_capture_name_trigger_exit = {
  CLI_NODE_KEYWORD,
  0,
  "exit",
  0,
  0,
  &node_gluon_capture_name_trigger_exit_end
  };


static cli_node_t node_gluon_capture_name_trigger_end = {
  CLI_NODE_END,
  CLI_NODE_FLAGS_OPT_END,
  cli_gluon_capture_name_trigger,
  0,
  0,
  &node_gluon_capture_name_trigger_eq
  };


static cli_node_t node_gluon_capture_name_trigger = {
  CLI_NODE_KEYWORD,
  0,
  "trigger",
  0,
  &node_gluon_capture_name_ls,
  &node_gluon_capture_name_trigger_end
  };


static cli_node_t node_gluon_capture_name_ls_end = {
  CLI_NODE_END,
  CLI_NODE_FLAGS_OPT_END,
  cli_gluon_capture_name_ls,
  0,
  0,
  0
  };


static cli_node_t node_gluon_capture_name_ls = {
  CLI_NODE_KEYWORD,
  0,
  "ls",
  0,
  &node_gluon_capture_name_id,
  &node_gluon_capture_name_ls_end
  };


static cli_node_t node_gluon_capture_name_id_id_name_end = {
  CLI_NODE_END,
  CLI_NODE_FLAGS_OPT_END,
  cli_gluon_capture_name_id_id_name,
  0,
  0,
  0
  };


static cli_node_t node_gluon_capture_name_id_id_name = {
  CLI_NODE_STRING,
  CLI_NODE_FLAGS_OPT_START | CLI_NODE_FLAGS_OPT_END | CLI_NODE_FLAGS_OPT_PARTIAL,
  "STRING:name",
  0,
  0,
  &node_gluon_capture_name_id_id_name_end
  };


static cli_node_t node_gluon_capture_name_id_id = {
  CLI_NODE_ENUM,
  0,
  &can_ids,
  0,
  0,
  &node_gluon_capture_name_id_id_name
  };


static cli_node_t node_gluon_capture_name_id = {
  CLI_NODE_KEYWORD,
  0,
  "id",
  0,
  &node_gluon_capture_name_rm,
  &node_gluon_capture_name_id_id
  };


static cli_node_t node_gluon_capture_name_rm_id_end = {
  CLI_NODE_END,
  CLI_NODE_FLAGS_OPT_END,
  cli_gluon_capture_name_rm_id,
  0,
  0,
  0
  };


static cli_node_t node_gluon_capture_name_rm_id = {
  CLI_NODE_ENUM,
  0,
  &can_ids,
  0,
  0,
  &node_gluon_capture_name_rm_id_end
  };


static cli_node_t node_gluon_capture_name_rm = {
  CLI_NODE_KEYWORD,
  0,
  "rm",
  0,
  &node_gluon_capture_name_exit,
  &node_gluon_capture_name_rm_id
  };


static cli_node_t node_gluon_capture_name_exit_end = {
  CLI_NODE_END,
  CLI_NODE_FLAGS_OPT_END,
  cli_gluon_capture_name_exit,
  0,
  0,
  0
  };


static cli_node_t node_gluon_capture_name_exit = {
  CLI_NODE_KEYWORD,
  0,
  "exit",
  0,
  0,
  &node_gluon_capture_name_exit_end
  };


static cli_node_t node_gluon_capture_name_end = {
  CLI_NODE_END,
  CLI_NODE_FLAGS_OPT_END,
  cli_gluon_capture_name,
  0,
  0,
  &node_gluon_capture_name_rate
  };


static cli_node_t node_gluon_capture_name = {
  CLI_NODE_STRING,
  0,
  "STRING:name",
  0,
  0,
  &node_gluon_capture_name_end
  };


static cli_node_t node_gluon_capture = {
  CLI_NODE_KEYWORD,
  0,
  "capture",
  0,
  &node_gluon_exit,
  &node_gluon_capture_name
  };


static cli_node_t node_gluon_exit_end = {
  CLI_NODE_END,
  CLI_NODE_FLAGS_OPT_END,
  cli_gluon_exit,
  0,
  0,
  0
  };


static cli_node_t node_gluon_exit = {
  CLI_NODE_KEYWORD,
  0,
  "exit",
  0,
  0,
  &node_gluon_exit_end
  };


static cli_node_t node_gluon_end = {
  CLI_NODE_END,
  CLI_NODE_FLAGS_OPT_END,
  cli_gluon,
  0,
  0,
  &node_gluon_ls
  };


static cli_node_t node_gluon = {
  CLI_NODE_KEYWORD,
  0,
  "gluon",
  0,
  0,
  &node_gluon_end
  };


cli_node_t gluon_cli_root = {
  CLI_NODE_ROOT,
  0,
  0,
  "Root node of the parser tree",
  0,
  &node_gluon
  };


