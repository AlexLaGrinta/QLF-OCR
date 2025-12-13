#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  GtkWidget *image_preview;
  char image_path[512];
  GtkWidget *window;
  GdkPixbuf *pixbuf_original;
  GdkPixbuf *pixbuf_display;
  GtkWidget *chk_median;
} AppWidgets;

static void on_open_clicked(GtkWidget *widget, gpointer data);
static void on_save_clicked(GtkWidget *widget, gpointer data);
static void on_ocr_clicked(GtkWidget *widget, gpointer data);
static void apply_dark_theme(GtkWidget *window);
static void on_fullscreen_toggle(GtkWidget *widget, gpointer data);

int main(int argc, char *argv[]) {
  gtk_init(&argc, &argv);
  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "QLF Searchwords Resolver");
  gtk_window_set_default_size(GTK_WINDOW(window), 1000, 700);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_container_set_border_width(GTK_CONTAINER(window), 20);
  g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
  apply_dark_theme(window);

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  GtkWidget *hbox_top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_box_pack_start(GTK_BOX(vbox), hbox_top, FALSE, FALSE, 0);

  GtkWidget *btn_open = gtk_button_new_with_label("Ouvrir une image PNG");
  GtkWidget *btn_ocr = gtk_button_new_with_label("OCR");
  GtkWidget *btn_save = gtk_button_new_with_label("Sauvegarder");
  // GtkWidget *btn_fullscreen = gtk_button_new_with_label("Plein écran");

  gtk_box_pack_start(GTK_BOX(hbox_top), btn_open, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox_top), btn_ocr, FALSE, FALSE, 0);

  // Median Filter Checkbox
  GtkWidget *chk_median = gtk_check_button_new_with_label("Filtre Médian");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chk_median),
                               TRUE); // Default ON
  gtk_box_pack_start(GTK_BOX(hbox_top), chk_median, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(hbox_top), btn_save, FALSE, FALSE, 0);
  // gtk_box_pack_end(GTK_BOX(hbox_top), btn_fullscreen, FALSE, FALSE, 0);

  GtkWidget *image_preview = gtk_image_new();
  gtk_widget_set_halign(image_preview, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(image_preview, GTK_ALIGN_CENTER);
  gtk_box_pack_start(GTK_BOX(vbox), image_preview, TRUE, TRUE, 0);

  AppWidgets *app = g_malloc(sizeof(AppWidgets));
  app->image_preview = image_preview;
  app->window = window;
  app->pixbuf_original = NULL;
  app->pixbuf_display = NULL;
  app->pixbuf_display = NULL;
  app->image_path[0] = '\0';
  app->chk_median = chk_median;

  g_signal_connect(btn_open, "clicked", G_CALLBACK(on_open_clicked), app);
  g_signal_connect(btn_save, "clicked", G_CALLBACK(on_save_clicked), app);
  g_signal_connect(btn_ocr, "clicked", G_CALLBACK(on_ocr_clicked), app);
  // g_signal_connect(btn_fullscreen, "clicked",
  // G_CALLBACK(on_fullscreen_toggle),app);

  gtk_widget_show_all(window);
  gtk_main();

  if (app->pixbuf_original)
    g_object_unref(app->pixbuf_original);
  if (app->pixbuf_display)
    g_object_unref(app->pixbuf_display);
  g_free(app);
  return 0;
}
/*
static void on_fullscreen_toggle(GtkWidget *widget, gpointer data) {
  AppWidgets *app = data;
  static gboolean full = FALSE;

  if (!app->pixbuf_display)
    return;

  if (!full) {
    gtk_window_fullscreen(GTK_WINDOW(app->window));
    gtk_button_set_label(GTK_BUTTON(widget), "Quitter plein écran");
  } else {
    gtk_window_unfullscreen(GTK_WINDOW(app->window));
    gtk_button_set_label(GTK_BUTTON(widget), "Plein écran");
  }

  full = !full;

  int win_width, win_height;
  gtk_window_get_size(GTK_WINDOW(app->window), &win_width, &win_height);

  if (app->pixbuf_display && app->pixbuf_original) {
    g_object_unref(app->pixbuf_display);
    GdkPixbuf *scaled =
        gdk_pixbuf_scale_simple(app->pixbuf_original, win_width - 40,
                                win_height - 80, GDK_INTERP_BILINEAR);
    app->pixbuf_display = scaled;
    gtk_image_set_from_pixbuf(GTK_IMAGE(app->image_preview),
                              app->pixbuf_display);
  }
}
*/
static void apply_dark_theme(GtkWidget *window) {
  GtkCssProvider *provider = gtk_css_provider_new();
  gtk_css_provider_load_from_data(
      provider,
      "window, button, entry, box "
      "{background-color:#1e1e1e;color:#f0f0f0;font-family:'Sans';font-size:"
      "14px;}"
      "button {background:#333;border-radius:8px;padding:6px "
      "10px;}button:hover {background:#555;}"
      "entry {background:#2b2b2b;border-radius:6px;color:#fff;border:1px solid "
      "#444;padding:4px;}",
      -1, NULL);
  gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                            GTK_STYLE_PROVIDER(provider),
                                            GTK_STYLE_PROVIDER_PRIORITY_USER);
  g_object_unref(provider);
}

static void on_open_clicked(GtkWidget *widget, gpointer data) {
  AppWidgets *app = data;
  GtkWidget *dialog = gtk_file_chooser_dialog_new(
      "Choisir une image", GTK_WINDOW(app->window),
      GTK_FILE_CHOOSER_ACTION_OPEN, "_Annuler", GTK_RESPONSE_CANCEL, "_Ouvrir",
      GTK_RESPONSE_ACCEPT, NULL);

  GtkFileFilter *filter_all = gtk_file_filter_new();
  gtk_file_filter_set_name(filter_all, "Images");
  gtk_file_filter_add_pattern(filter_all, "*.png");
  gtk_file_filter_add_pattern(filter_all, "*.jpg");
  gtk_file_filter_add_pattern(filter_all, "*.jpeg");
  gtk_file_filter_add_pattern(filter_all, "*.bmp");
  gtk_file_filter_add_pattern(filter_all, "*.gif");
  gtk_file_filter_add_pattern(filter_all, "*.tiff");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_all);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    char *file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    strncpy(app->image_path, file, sizeof(app->image_path) - 1);
    app->image_path[sizeof(app->image_path) - 1] = '\0';
    g_free(file);

    if (app->pixbuf_original)
      g_object_unref(app->pixbuf_original);
    app->pixbuf_original = gdk_pixbuf_new_from_file(app->image_path, NULL);
    if (!app->pixbuf_original) {
      g_print("Erreur: impossible de charger l'image.\n");
      gtk_widget_destroy(dialog);
      return;
    }

    if (app->pixbuf_display)
      g_object_unref(app->pixbuf_display);
    app->pixbuf_display = gdk_pixbuf_copy(app->pixbuf_original);
    gtk_image_set_from_pixbuf(GTK_IMAGE(app->image_preview),
                              app->pixbuf_display);
  }
  gtk_widget_destroy(dialog);
}

static void on_save_clicked(GtkWidget *widget, gpointer data) {
  AppWidgets *app = data;
  if (!app->pixbuf_display)
    return;

  GtkWidget *dialog = gtk_file_chooser_dialog_new(
      "Enregistrer l'image", GTK_WINDOW(app->window),
      GTK_FILE_CHOOSER_ACTION_SAVE, "_Annuler", GTK_RESPONSE_CANCEL,
      "_Enregistrer", GTK_RESPONSE_ACCEPT, NULL);

  GtkFileFilter *filter = gtk_file_filter_new();
  gtk_file_filter_add_pattern(filter, "*.bmp");
  gtk_file_filter_add_pattern(filter, "*.png");
  gtk_file_filter_add_pattern(filter, "*.jpg");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

  gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog),
                                                 TRUE);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

    const char *ext = strrchr(filename, '.');
    const char *type = "bmp";
    if (ext) {
      if (strcasecmp(ext, ".png") == 0)
        type = "png";
      else if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0)
        type = "jpg";
    }

    GError *err = NULL;
    if (!gdk_pixbuf_save(app->pixbuf_display, filename, type, &err, NULL)) {
      g_print("Erreur lors de la sauvegarde : %s\n", err->message);
      g_error_free(err);
    }

    g_free(filename);
  }

  gtk_widget_destroy(dialog);
}

static void on_ocr_clicked(GtkWidget *widget, gpointer data) {
  AppWidgets *app = data;
  if (!app->pixbuf_original || strlen(app->image_path) == 0)
    return;

  gboolean use_median =
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(app->chk_median));
  char command[2048];
  snprintf(command, sizeof(command), "cd .. && ./ocr \"%s\" %d",
           app->image_path, use_median ? 1 : 0);

  g_print("Executing: %s\n", command);
  int ret = system(command);

  if (ret != 0) {
    g_print("Error executing OCR solver (code %d).\n", ret);
    // Could show a dialog here
    return;
  }

  // Result should be in "../output/solved.bmp"
  const char *result_path = "../output/solved.bmp";

  if (app->pixbuf_display)
    g_object_unref(app->pixbuf_display);

  GError *error = NULL;
  app->pixbuf_display = gdk_pixbuf_new_from_file(result_path, &error);

  if (!app->pixbuf_display) {
    g_print("Error loading result: %s\n", error ? error->message : "unknown");
    if (error)
      g_error_free(error);
    return;
  }

  gtk_image_set_from_pixbuf(GTK_IMAGE(app->image_preview), app->pixbuf_display);
  g_print("OCR finished. Displaying result.\n");
}
