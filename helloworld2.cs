// helloworld.cs - Gtk# Tutorial example
 
using Gtk;
using GtkSharp;
using System;
using System.Drawing;
 
public class HelloWorld {
        static void hello (object obj, EventArgs args)
        {
                Console.WriteLine("Hello World");
                Application.Quit ();
        }
 
        static void delete_event (object obj, DeleteEventArgs args)
        {
                    Console.WriteLine ("delete event occurred\n");
                    Application.Quit ();
        }
 
        public static void Main(string[] args)
        {
                Application.Init ();
                Window window = new Window ("helloworld");
                window.DeleteEvent += new DeleteEventHandler (delete_event);
                window.BorderWidth = 10;
                Button btn = new Button ("Hello World");
                btn.Clicked += new EventHandler (hello);
                window.Add (btn);
                window.ShowAll ();
                Application.Run ();
        }
}
