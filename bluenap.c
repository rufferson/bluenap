#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <bluetooth/bnep.h>

#include <linux/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <linux/if_bridge.h>

#include <dbus/dbus.h>

#include <signal.h>

#define BRNAME_DEFAULT "nap"
#define SRV_DEFAULT "gn"
#define DEST "org.bluez"
#define PATH "/org/bluez/hci0"
#define IFACE "org.bluez.NetworkServer1"
#define METH "Register"
#define MATCH_IFREMOVE "type='signal',path='/',interface='org.freedesktop.DBus.ObjectManagerr',member='InterfacesRemoved',arg0path='/org/bluez/'"


static int run = 1;
static void handler(int signo) {
        printf("Signal caught, attempting to exit\n");
        run = 0;
}
void print_msg(DBusMessage *msg, const char *from, DBusError *err) {
    char *val;
    DBusMessageIter i,si,ssi;
    printf("%s got: %s %s.%s\n",from,dbus_message_get_path(msg), dbus_message_get_interface(msg),dbus_message_get_member(msg));
    if (!dbus_message_get_args(msg, err, DBUS_TYPE_STRING, &val, DBUS_TYPE_INVALID)) {
        fprintf(stderr,"Can't get args: %s\n", err->message);
        return;
    }
    printf("Signal[%s]: %s\n",dbus_message_get_signature(msg),val);
    dbus_message_iter_init(msg,&i);
    fprintf(stderr,"Arg[%s]\n",dbus_message_iter_get_signature(&i));
    dbus_message_iter_next(&i);
    fprintf(stderr,"Arg[%s]\n",dbus_message_iter_get_signature(&i));
    if(dbus_message_iter_get_arg_type(&i)==DBUS_TYPE_ARRAY) {
        for(dbus_message_iter_recurse(&i,&si);dbus_message_iter_get_arg_type(&si) != DBUS_TYPE_INVALID;dbus_message_iter_next(&si)) {
            fprintf(stderr,"Sub Arg[%s]\n",dbus_message_iter_get_signature(&si));
            if(dbus_message_iter_get_arg_type(&si)==DBUS_TYPE_DICT_ENTRY) {
                dbus_message_iter_recurse(&si,&ssi);
                dbus_message_iter_get_basic(&ssi, &val);
                fprintf(stderr,"S-sub Arg[%s]=%s\n",dbus_message_iter_get_signature(&ssi),val);
            }
        }
    }
}
/*
signal sender=:1.61 -> dest=(null destination) serial=46 path=/org/bluez/hci0; interface=org.freedesktop.DBus.Properties; member=PropertiesChanged
   string "org.bluez.Adapter1"
   array [
      dict entry(
         string "Powered"
         variant             boolean false
      )
      dict entry(
         string "Discovering"
         variant             boolean false
      )
      dict entry(
         string "Class"
         variant             uint32 0
      )
   ]
   array [
   ]
signal sender=:1.2 -> dest=(null destination) serial=407 path=/; interface=org.freedesktop.DBus.ObjectManager; member=InterfacesRemoved
   object path "/org/bluez/hci0"
   array [
      string "org.freedesktop.DBus.Properties"
      string "org.freedesktop.DBus.Introspectable"
      string "org.bluez.Adapter1"
      string "org.bluez.NetworkServer1"
   ]

*/
int main(int argc, char * argv[]) {
        int ret,brs;
        DBusConnection *dbc;
        DBusMessage *msg,*rep;
        DBusError err;
        char brname[IFNAMSIZ] = BRNAME_DEFAULT, srv[5]=SRV_DEFAULT,filter[DBUS_MAXIMUM_MATCH_RULE_LENGTH];
        const char *v_brn=brname, *v_srv=srv, *v;
        struct sigaction sa;

        printf("Opening control socket\n");
        if ((ret = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0)
                goto err1;
        brs=ret;
        printf("Adding bridge %s\n",brname);
        ret = ioctl(brs, SIOCBRADDBR, brname);
        if(ret < 0 && errno != EEXIST)
                goto err2;
        printf("Attaching to system bus\n");
        dbus_error_init (&err);
        dbc=dbus_bus_get(DBUS_BUS_SYSTEM,&err);
        if(dbc==NULL)
                goto err3;
        printf("Registering network service %s\n",srv);
        if((msg=dbus_message_new_method_call(DEST,PATH,IFACE,METH))==NULL)
                goto err4;
        if(!dbus_message_append_args(msg,DBUS_TYPE_STRING,&v_srv,DBUS_TYPE_STRING,&v_brn,DBUS_TYPE_INVALID))
                goto err5;
        if((rep=dbus_connection_send_with_reply_and_block(dbc,msg,-1,&err))==NULL)
                goto err5;
        v=dbus_message_get_error_name(rep);
        if(v) {
                printf("Reply: %s\n",v);
                goto err6;
        }
        printf("Setting dbus filters\n");
        dbus_bus_add_match(dbc,MATCH_IFREMOVE,&err);
        snprintf(filter,DBUS_MAXIMUM_MATCH_RULE_LENGTH,"type='signal',path='%s'",PATH);
        dbus_bus_add_match(dbc,filter,&err);
        if (dbus_error_is_set (&err)) {
                printf("DBus failed setting filter <%s>: %s\n",filter,err.message);
                goto err6;
        }
        /*if (!dbus_connection_add_filter (dbc, filter_func, NULL, NULL)) {
                fprintf (stderr, "Couldn't add filter!\n");
                goto err6;
        }*/
        fprintf(stderr,"Installing signal hooks\n");
        sa.sa_handler=handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        if((ret=sigaction(SIGINT,&sa,NULL))<0)
                        goto err6;
        printf("Bridge is open, hanging on the bus\n");
        while (dbus_connection_get_is_connected(dbc) && dbus_connection_read_write_dispatch (dbc, 333) && run)
        {
                if(rep)
                    dbus_message_unref(rep);
                rep=dbus_connection_pop_message(dbc);
                if(rep)
                        print_msg(rep, "Loop",&err);
        }
        printf("Exiting...\n");
    err6:
        if(rep)
            dbus_message_unref(rep);
    err5:
        dbus_message_unref(msg);
    err4:
        dbus_connection_unref(dbc);
        dbus_error_free(&err);
    err3:
        ret = ioctl(brs, SIOCBRDELBR, brname);
    err2:
        close(brs);
    err1:
        perror("Err");
        exit(ret);
}
