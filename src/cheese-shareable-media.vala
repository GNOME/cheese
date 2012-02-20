/*
 * Copyright © 2011 Patricia Santana Cruz <patriciasantanacruz@gmail.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * Class used for sharing images or videos with different technologies through
 * nautilus-sendto.
 */
public class Cheese.ShareableMedia : GLib.Object {
    private uint num_children = 0;
    Cheese.MainWindow window;

    /**
     * Create a new ShareableMedia instance.
     *
     * @param main_window the main window of the Cheese application
     */
    public ShareableMedia (Cheese.MainWindow main_window)
    {
        window = main_window;
    }

    /**
     * Callback to be called when the child exits.
     *
     * Cleans up the child process spawned by {@link ShareableMedia.share_files}
     * and if that child is the last one, it will also set the cursor
     * back to normal.
     *
     * @param pid the process id of the child process
     * @param status the status information about the child process
     *
     * @see ShareableMedia.share_files
     */
    void child_finished (Pid pid, int status)
    {
        /* Method for checking WIFEXITED(status) value, which returns true if the
         * child terminated normally, that is, by calling exit(3) or _exit(2), or
         * by returning from main().
         */
        if (!Process.if_exited(status))
            warning ("nautilus-sendto exited abnormally");

        Process.close_pid(pid);

        --num_children;
        if (num_children == 0)
        {
            window.get_window ().set_cursor (
                new Gdk.Cursor (Gdk.CursorType.LEFT_PTR));
        }
    }

    /**
     * Used for sharing one or more images and/or one or more videos through
     * different technologies.
     *
     * It works asynchronously and will create a new child every time it is
     * called. When the child exists, the {@link ShareableMedia.child_finished}
     * callback is called.
     *
     * It will also set the cursor to busy to indicate to the user that
     * there is some task that is being done.
     *
     * @param files a list of the files (videos and/or images that are going to
     * be shared.
     *
     * @see ShareableMedia.child_finished.
     */
    public void share_files (GLib.List<GLib.File> files)
    {
        string[] argv = {};
        // Defined in cheese-window.vala.
        argv += SENDTO_EXEC;

        files.foreach ((file) => argv += file.get_path ());

        try
        {
            Pid child_pid;

            if (Process.spawn_async ("/", argv, null,
                SpawnFlags.SEARCH_PATH | SpawnFlags.DO_NOT_REAP_CHILD, null,
                out child_pid))
            {
                ChildWatch.add (child_pid, child_finished);

                ++num_children;
                /* Show a busy cursor if there are any children. It just needs
                 * to be done for the first child.
                 */
                if (num_children == 1)
                {
                    window.get_window ().set_cursor (new Gdk.Cursor
                        (Gdk.CursorType.WATCH));
                }
            }
        }
        catch (SpawnError error)
        {
            window.get_window ().set_cursor (new Gdk.Cursor
                (Gdk.CursorType.LEFT_PTR));
            warning ("Unable to launch nautilus-sendto: %s\n", error.message);
        }
    }
}
