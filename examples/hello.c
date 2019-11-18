

#include <stdio.h>
#include <stdlib.h>

#include "fxc.h"
#include "fxbutton.h"

void buttonClick(struct dkWindow *win, void *udata)
{
  dkAppExit((struct dkApp *)udata, 0);
}

int main(int argc, char *argv[])
{
  struct dkApp *app;
  struct dkTopWindow *mainwindow;
  struct dkButton *bt;

  app = dkAppNew();
  dkAppInit(app, argc, argv);

  mainwindow = dkMainWindowNew(app, "Hello");

  bt = dkButtonNew((struct dkWindow *)mainwindow, "&Hello, World!");
  dkWindowSetActionCallback((struct dkWindow *)bt, buttonClick, app);

  dkAppCreate(app);

  /* Now show the window */
  dkWindowShowAndPlace((struct dkWindow *)mainwindow, PLACEMENT_SCREEN);

  dkAppRun(app);
  dkAppDel(app);
  return 0;
}

