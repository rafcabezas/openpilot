#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cassert>
#include <sys/resource.h>

#include "visionbuf.h"
#include "visionipc_client.h"
#include "common/swaglog.h"
#include "common/util.h"

#include "models/dmonitoring.h"

#ifndef PATH_MAX
#include <linux/limits.h>
#endif

bool get_bool_config_option(const char *key) {
  char line[500];
  FILE *stream;
  stream = fopen("/data/bb_openpilot.cfg", "r");
  while(fgets(line, 500, stream) != NULL)
  {
          char setting[256], value[256], oper[10];
          if(line[0] == '#') continue;
          if(sscanf(line, "%s %s %s", setting, oper, value) != 3) {
            continue;
          }       
          if ((strcmp(key, setting) == 0) && (strcmp("True", value) == 0)) {
            fclose(stream);
            return true;
          }
  }
  fclose(stream);
  return false;
}

ExitHandler do_exit;

int main(int argc, char **argv) {
  setpriority(PRIO_PROCESS, 0, -15);

  PubMaster pm({"driverState"});

  // init the models
  DMonitoringModelState dmonitoringmodel;
  dmonitoring_init(&dmonitoringmodel);

  VisionIpcClient vipc_client = VisionIpcClient("camerad", VISION_STREAM_YUV_FRONT, true);
  while (!do_exit){
    if (!vipc_client.connect(false)){
      util::sleep_for(100);
      continue;
    }
    break;
  }

  while (!do_exit) {
    LOGW("connected with buffer size: %d", vipc_client.buffers[0].len);

    bool is_dm_throttled = get_bool_config_option("throttle_driver_monitor");
    bool is_dm_disabled = !get_bool_config_option("enable_driver_monitor");
    double last = 0;
    while (!do_exit) {
      VisionIpcBufExtra extra = {0};
      VisionBuf *buf = vipc_client.recv(&extra);
      if (buf == nullptr){
        continue;
      }

      double t1 = millis_since_boot();
      DMonitoringResult res = dmonitoring_eval_frame(&dmonitoringmodel, buf->addr, buf->width, buf->height);
      double t2 = millis_since_boot();

      // send dm packet
      dmonitoring_publish(pm, extra.frame_id, res, (t2-t1)/1000.0, dmonitoringmodel.output);

      LOGD("dmonitoring process: %.2fms, from last %.2fms", t2-t1, t1-last);
      last = t1;
      if (is_dm_disabled) {
        usleep(1*1000*1000);
      } else if (is_dm_throttled) {
        usleep(250*1000);
      } else {
#ifdef QCOM2
      // this makes it run at about 2.7Hz on tici CPU to deal with modeld lags
      // TODO: DSP needs to be freed (again)
      usleep(250000);
#endif
      }
    }
  }

  dmonitoring_free(&dmonitoringmodel);

  return 0;
}
