/*
 Compile this like so, or similar:
   gcc postgres_gda_test.c `pkg-config libgda --libs --cflags`
 murrayc
*/


#include <libgda/libgda.h>


/* Show errors from a working connection */
static void
get_errors (GdaConnection *connection)
{
        GList    *list;
        GList    *node;
        GdaError *error;
      
        list = (GList *) gda_connection_get_errors (connection);
      
        for (node = g_list_first (list); node != NULL; node = g_list_next (node)) {
		error = (GdaError *) node->data;
		g_print ("Error no: %d\t", gda_error_get_number (error));
		g_print ("desc: %s\t", gda_error_get_description (error));
		g_print ("source: %s\t", gda_error_get_source (error));
		g_print ("sqlstate: %s\n", gda_error_get_sqlstate (error));
	}
}

int
main(int argc, char *argv[])
{
        const gchar* connection_string = "HOST=localhost;USER=murrayc;PASSWORD=yourpasswordhere;DATABASE=template1";
        GdaClient     *client = 0;
        GdaConnection *con = 0;
        
        gboolean       errors = FALSE;
        gint           rows;

        gda_init ("glom-gda-test", NULL, argc, argv);

        /* 3. Create a gda client */
        client = gda_client_new ();
  
        /* 4. Open the connection */
        con = gda_client_open_connection_from_string (client, "PostgreSQL", connection_string, 0);
        if (!GDA_IS_CONNECTION (con)) {
		g_print ("** ERROR: could not open connection.\n");
                /* This can not work because it needs a working connection: get_errors (con); */
		return 0;
	}

        gboolean created = gda_connection_create_database(con, "glomtest");
	if(!created)
	  g_print("** Error: gda_connection_create_database failed.\n");
     
	gda_connection_close (con);

	g_object_unref (G_OBJECT (client));

        g_print ("** Connection successfully opened, database created, and connection closed.\n");

        return 0;
}


