

#include <stdio.h>
#include <stdlib.h>

#include "fxc.h"
#include "fxhorizontalframe.h"
#include "fxtextfield.h"
#include "fxverticalframe.h"
#include "fxtext.h"

int main(int argc, char *argv[])
{
  struct dkApp *app;
  struct dkTopWindow *mainwindow;
  struct dkWindow *hf;
  struct dkWindow *vf, *textbox;
//  struct dkText *txt;

  /* Create a new dkApplication object */
  app = dkAppNew();

  /* Initialize the dkApp object, and pass any arguments for processing
   * to the DTK engine. */
  dkAppInit(app, argc, argv);

  /* Construct our first top level window.
   * Here we specify 800 x 600 as the initial size */
  mainwindow = dkMainWindowNewEx(app, "ONC", DECOR_ALL, 0, 0, 800, 600, 0, 0, 0, 0, 0, 0);

  /* Construct a horizontal and vertical frame. */
  hf = dkHorizontalFrameNew((struct dkWindow *)mainwindow, LAYOUT_FILL_X | LAYOUT_FILL_Y);
  vf = dkVerticalFrameNewEx(hf, LAYOUT_FILL_X | LAYOUT_FILL_Y,
      0, 0, 0, 0, 0, 0, 0, 0, DEFAULT_SPACING, DEFAULT_SPACING);

  /* Construct a frame for the text window */
  textbox = dkVerticalFrameNewEx(vf,
      LAYOUT_FILL_X | LAYOUT_FILL_Y | FRAME_SUNKEN | FRAME_THICK, 0, 0, 0, 0, 0, 0, 0, 0,
      DEFAULT_SPACING, DEFAULT_SPACING);

  dkTextNew((struct dkComposite *)textbox, NULL, 0,
      FRAME_SUNKEN | LAYOUT_FILL_X | LAYOUT_FILL_Y | TEXT_READONLY | TEXT_WORDWRAP | TEXT_SHOWACTIVE | TEXT_AUTOSCROLL,
      0, 0, 0, 0, 3, 3, 2, 2);
  /* Construct a text entry box and put it at the bottom of the vertical frame */
  dkTextFieldNew((struct dkComposite *)vf, 25, FRAME_NORMAL | LAYOUT_FILL_X | LAYOUT_BOTTOM);

  /* Call dkAppCreate to actually create all the windows that we've constructed. */
  dkAppCreate(app);

  /* Now show the window */
  dkWindowShowAndPlace((struct dkWindow *)mainwindow, PLACEMENT_SCREEN);

  /* Run our event loop */
  dkAppRun(app);

  /* After the user closes the main window, dkAppDel will cleanup all windows and the
   * application object */
  dkAppDel(app);

  return 0;
}

