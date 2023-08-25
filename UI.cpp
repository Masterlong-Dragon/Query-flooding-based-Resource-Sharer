//
// Created by MasterLong on 2023/6/9.
//

#include "UI.h"
#include "fileHelper.h"

namespace flood {

    static GtkApplication *app;
    static GtkWidget *window;
    static int status;
    static Peer peer;
    // 回调函数
    static std::function<Peer()> getPeerInfo;
    static std::function<int()> getDownloadCount;
    static std::function<int()> getServingCount;

    const char *ui_tab_names[] = {"Query", "Download", "Config"};

    typedef struct msgData {
        const char *msg;
        int tab_index;
    } msgData;

    static int dialog_cnt = 0;

    static void showDialog(std::string tittle, std::string msg) {
        GtkAlertDialog *dialog;

        dialog = gtk_alert_dialog_new(("[" + std::to_string(dialog_cnt++) + "]" + tittle).c_str());
        gtk_alert_dialog_set_detail(dialog,
                                    msg.c_str());
        gtk_alert_dialog_show(dialog, GTK_WINDOW(window));
        g_object_unref(dialog);
    }

    void UI::addMessage(std::string msg, const int tab_index) {
        g_idle_add(+[](gpointer user_data) {
            msgData *data = (msgData *) user_data;
            showDialog(ui_tab_names[data->tab_index], data->msg);
            free(data);
            return FALSE;
        }, new msgData{msg.c_str(), tab_index});
    }


    // 回调函数：设置列表项
    static void setup_cb(GtkSignalListItemFactory *factory, GtkListItem *listitem, gpointer user_data) {
        GtkWidget *label = gtk_label_new(NULL);
        gtk_list_item_set_child(listitem, label);
    }

// 回调函数：绑定列表项
    static void bind_cb(GtkSignalListItemFactory *factory, GtkListItem *listitem, gpointer user_data) {
        GtkWidget *lb = gtk_list_item_get_child(listitem);
        /* Strobj is owned by the instance. Caller mustn't change or destroy it. */
        GtkStringObject *strobj = (GtkStringObject *) gtk_list_item_get_item(listitem);
        /* The string returned by gtk_string_object_get_string is owned by the instance. */
        gtk_label_set_text(GTK_LABEL (lb), gtk_string_object_get_string(strobj));
    }

    static void unbind_cb(GtkSignalListItemFactory *self, GtkListItem *listitem, gpointer user_data) {
        /* There's nothing to do here. */
    }

// 回调函数：销毁列表项
    static void teardown_cb(GtkSignalListItemFactory *factory, GtkListItem *listitem, gpointer user_data) {
        // 无需进行任何操作
    }


    void UI::activate(GtkApplication *app, gpointer user_data) {
        // 创建主窗口
        window = gtk_application_window_new(app);

        gtk_window_set_title(GTK_WINDOW(window), "洪泛文件查询器");
        gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

        // 创建一个堆叠容器
        GtkWidget *stack = gtk_stack_new();
        // 创建切换器
        GtkWidget *querySwitcher = gtk_stack_switcher_new();
        gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
        gtk_stack_set_transition_duration(GTK_STACK(stack), 250);
        gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(querySwitcher), GTK_STACK(stack));

        // 切换器和堆叠容器 上下布局
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_box_append(GTK_BOX(vbox), querySwitcher);
        gtk_box_append(GTK_BOX(vbox), stack);

        //
        gtk_window_set_child(GTK_WINDOW(window), vbox);


        // 创建并添加查询页面
        // 创建切换用的switcher
        GtkWidget *queryPage = createQueryPage();
        gtk_stack_add_titled(GTK_STACK(stack), queryPage, "查询", "查询");

        // 创建并添加下载页面
        GtkWidget *downloadPage = createDownloadPage();
        gtk_stack_add_titled(GTK_STACK(stack), downloadPage, "下载", "下载");

        // 创建并添加配置页面
        GtkWidget *configPage = createConfigPage();
        gtk_stack_add_titled(GTK_STACK(stack), configPage, "配置", "配置");

        // 创建并添加状态页面
        GtkWidget *statusPage = createStatusPage();
        gtk_stack_add_titled(GTK_STACK(stack), statusPage, "说明", "说明");


        gtk_window_present(GTK_WINDOW(window));
    }

    void UI::run() {
        app = gtk_application_new("io.masterlong.netproj", G_APPLICATION_NON_UNIQUE);
        g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
        status = g_application_run(G_APPLICATION(app), 0, NULL);
        g_object_unref(app);
    }


// 发送查询和查看查询结果页面

// 关键数据结构
    static GtkWidget *queryEntry;
    static std::function<void(std::string)> sendQuery;

    // 发送查询按钮的点击事件处理函数
    void UI::onQueryButtonClicked(GtkButton *button, gpointer data) {
        // 获取查询关键字
        const char *query = gtk_editable_get_text(GTK_EDITABLE(queryEntry));
        if (strlen(query) == 0) {
            showDialog("Query", "查询关键字不能为空");
            return;
        }
        // 发送查询
        sendQuery(query);
        showDialog("Query", "已发送查询内容：" + std::string(query));
    }

    static GtkStringList *s = NULL;
    static std::vector<GtkStringList *> rslist;

    static GtkSingleSelection *sm;
    static GtkSingleSelection *rsm;

    static std::mutex query_hit_list_mutex;
    static std::vector<QueryHit> query_hit_list;

    static std::function<void(const QueryHit &query_hit, const int query_hit_index)> download;

    static void on_selection_changed(GtkSingleSelection *selection, gpointer user_data) {
        // 获取选中的列表项
        int index = gtk_single_selection_get_selected(selection);
        // 更新模型
        gtk_single_selection_set_model(GTK_SINGLE_SELECTION(rsm), G_LIST_MODEL(rslist[index]));
    }

    static void downloadFile(GtkButton *button, gpointer user_data);

    GtkWidget *UI::createQueryPage() {
        // 创建主容器
        GtkWidget *queryPage = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);

        // 创建查询输入框
        queryEntry = gtk_entry_new();
        gtk_box_append(GTK_BOX(queryPage), queryEntry);
        // 提示词
        gtk_entry_set_placeholder_text(GTK_ENTRY(queryEntry), "请输入查询关键字");
        // margin
        gtk_widget_set_margin_start(queryEntry, 20);
        gtk_widget_set_margin_end(queryEntry, 20);
        gtk_widget_set_margin_top(queryEntry, 5);

        // 创建发送查询按钮
        GtkWidget *queryButton = gtk_button_new_with_label("发送查询");
        gtk_box_append(GTK_BOX(queryPage), queryButton);
        // margin
        gtk_widget_set_margin_start(queryButton, 20);
        gtk_widget_set_margin_end(queryButton, 20);
        gtk_widget_set_margin_top(queryButton, 5);
        gtk_widget_set_margin_bottom(queryButton, 5);

        // 绑定发送查询按钮的点击事件
        g_signal_connect(queryButton, "clicked", G_CALLBACK(onQueryButtonClicked), NULL);

        // 创建查询结果列表
        GtkWidget *select_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
        gtk_widget_set_size_request(select_box, 400, 400);
        gtk_widget_set_vexpand(select_box, TRUE);
        gtk_widget_set_hexpand(select_box, TRUE);
        gtk_widget_set_margin_start(select_box, 20);
        gtk_widget_set_margin_end(select_box, 20);
        gtk_box_append(GTK_BOX(queryPage), select_box);

        // 下载确认按钮
        GtkWidget *downloadButton = gtk_button_new_with_label("下载");
        gtk_widget_set_margin_start(downloadButton, 20);
        gtk_widget_set_margin_end(downloadButton, 20);
        gtk_widget_set_margin_top(downloadButton, 20);
        gtk_widget_set_margin_bottom(downloadButton, 20);
        gtk_box_append(GTK_BOX(queryPage), downloadButton);

        // 两个列表项
        {
            GtkWidget *query_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
            gtk_widget_set_vexpand(query_box, TRUE);
            gtk_widget_set_hexpand(query_box, TRUE);
            gtk_widget_set_size_request(query_box, 400, 300);
            gtk_widget_set_margin_start(query_box, 5);
            gtk_widget_set_margin_end(query_box, 5);

            // label
            GtkWidget *q_label = gtk_label_new("查询结果");
            gtk_box_append(GTK_BOX(query_box), q_label);
            gtk_box_append(GTK_BOX(select_box), query_box);

            GtkWidget *res_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
            gtk_widget_set_vexpand(res_box, TRUE);
            gtk_widget_set_hexpand(res_box, TRUE);
            gtk_widget_set_size_request(res_box, 400, 300);
            gtk_widget_set_margin_start(res_box, 5);
            gtk_widget_set_margin_end(res_box, 5);

            // label
            GtkWidget *r_label = gtk_label_new("文件列表");
            gtk_box_append(GTK_BOX(res_box), r_label);
            gtk_box_append(GTK_BOX(select_box), res_box);

            // 创建一个滚动窗口和列表用于显示查询结果
            GtkWidget *scrolled = gtk_scrolled_window_new();
            gtk_widget_set_size_request(scrolled, 400, 300);
            gtk_widget_set_vexpand(scrolled, TRUE);
            gtk_box_append(GTK_BOX(query_box), scrolled);

            GtkWidget *res_scrolled = gtk_scrolled_window_new();
            gtk_widget_set_size_request(res_scrolled, 400, 300);
            gtk_widget_set_vexpand(res_scrolled, TRUE);
            gtk_box_append(GTK_BOX(res_box), res_scrolled);

            /* sl is owned by ns */
            /* ns and factory are owned by lv. */
            /* Therefore, you don't need to care about their destruction. */

            sm = gtk_single_selection_new(NULL);

            GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
            g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), NULL);
            g_signal_connect(factory, "bind", G_CALLBACK(bind_cb), NULL);
            /* The following two lines can be left out. The handlers do nothing. */
            g_signal_connect(factory, "unbind", G_CALLBACK(unbind_cb), NULL);
            g_signal_connect(factory, "teardown", G_CALLBACK(teardown_cb), NULL);

            GtkWidget *lv = gtk_list_view_new(GTK_SELECTION_MODEL(sm), factory);
            gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), lv);

            // 创建一个滚动窗口和列表用于显示查询结果

            rsm = gtk_single_selection_new(NULL);
            GtkListItemFactory *res_factory = gtk_signal_list_item_factory_new();
            g_signal_connect(res_factory, "setup", G_CALLBACK(setup_cb), NULL);
            g_signal_connect(res_factory, "bind", G_CALLBACK(bind_cb), NULL);
            /* The following two lines can be left out. The handlers do nothing. */
            g_signal_connect(res_factory, "unbind", G_CALLBACK(unbind_cb), NULL);
            g_signal_connect(res_factory, "teardown", G_CALLBACK(teardown_cb), NULL);

            GtkWidget *res_lv = gtk_list_view_new(GTK_SELECTION_MODEL(rsm), res_factory);
            gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(res_scrolled), res_lv);

            // 获取选中
            g_signal_connect(sm, "selection-changed", G_CALLBACK(on_selection_changed), NULL);

            // 下载按钮
            g_signal_connect(downloadButton, "clicked", G_CALLBACK(downloadFile), NULL);
        }
        return queryPage;
    }

    static gboolean updateQueries(gpointer data) {
        // 清除原有的查询结果
        gtk_single_selection_set_model(GTK_SINGLE_SELECTION(sm), NULL);
        gtk_single_selection_set_model(GTK_SINGLE_SELECTION(rsm), NULL);

        for (auto &rs: rslist) {
            g_object_unref(rs);
        }
        rslist.clear();
        if (s != NULL)
            g_object_unref(s);

        // 更新query列表
        s = gtk_string_list_new(NULL);
        for (auto &query_hit: query_hit_list) {
            gtk_string_list_append(s, (query_hit.getPeerAddress() + ":" +
                                       std::to_string(query_hit.getPeerPort())).c_str());
        }

        // 更新result列表
        for (auto &query_hit: query_hit_list) {
            GtkStringList *rs = gtk_string_list_new(NULL);
            for (auto &file: query_hit.getFileName()) {
                gtk_string_list_append(rs, file.first.c_str());
            }
            rslist.push_back(rs);
        }

        // 最后更新model
        gtk_single_selection_set_model(GTK_SINGLE_SELECTION(sm), G_LIST_MODEL(s));
        gtk_single_selection_set_model(GTK_SINGLE_SELECTION(rsm), G_LIST_MODEL(rslist[0]));
        return FALSE;
    }

    void UI::QueryCallback(std::unordered_map<boost::asio::ip::tcp::endpoint, QueryHit> query_hit_map) {
        query_hit_list_mutex.lock();
        query_hit_list.clear();
        for (auto &query_hit: query_hit_map) {
            query_hit_list.push_back(query_hit.second);
        }
        g_idle_add((GSourceFunc) updateQueries, NULL);
        query_hit_list_mutex.unlock();
    }

    // 相关数据结构
    typedef struct progressMsg {
        int id;
        double fraction;
    } progressMsg;

    typedef struct finishMsg {
        int id;
        bool success;
    } finishMsg;

    static GtkWidget *download_list;
    static std::unordered_map<int, std::pair<std::string, GtkWidget *>> download_progressbars;
    static std::unordered_map<GtkWidget *, finishMsg> terminated_map;

    static GtkWidget *createProgressBar(const char *filename) {
        GtkWidget *progressBar = gtk_progress_bar_new();
        gtk_widget_set_hexpand(progressBar, TRUE);
        gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progressBar), TRUE);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progressBar), (std::string(filename) + "|下载中...").c_str());
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressBar), 0.0);
        // 大小
        gtk_widget_set_size_request(progressBar, 400, 30);

        // 添加到列表
        gtk_list_box_prepend(GTK_LIST_BOX(download_list), progressBar);
        return progressBar;
    }

    static void downloadFile(GtkButton *button, gpointer user_data) {
        // 获取选中的列表项
        int q_index = gtk_single_selection_get_selected(sm);
        int r_index = gtk_single_selection_get_selected(rsm);
        // 检查是否选中
        if (q_index == -1 || r_index == -1) {
            showDialog("Download", "请先选择要下载的文件");
            return;
        }
        // 获取文件名
        QueryHit &query_hit = query_hit_list[q_index];
        auto files = query_hit.getFileName();
        std::string file_path = files[r_index].second;
        std::string file_name = files[r_index].first;
        // 创建进度条
        GtkWidget *progress_bar = createProgressBar(file_name.c_str());
        // 添加到map
        download_progressbars[getDownloadCount()] = std::make_pair(file_name, progress_bar);
        // 下载文件
        download(query_hit, r_index);
        // 开始下载
        showDialog("Download", "开始下载文件: " + file_name);
    }

    // 实在不知道怎么解决下面传参的内存问题了, 只能用全局变量了
    static std::string temp_path;

    static void showFile(GtkListBox *self,
                         GtkListBoxRow *row,
                         gpointer user_data) {
        // 获取文件名
        GtkWidget *progressBar = gtk_list_box_row_get_child(row);
        std::string filename = gtk_progress_bar_get_text(GTK_PROGRESS_BAR(progressBar));
        filename = filename.substr(0, filename.find('|'));
        if (terminated_map.find(progressBar) == terminated_map.end()) {
            showDialog("Download", "文件: " + filename + "未下载完成");
            return;
        }
        if (!terminated_map[progressBar].success) {
            showDialog("Download", "文件: " + filename + "下载失败");
            return;
        }
        temp_path = peer.getDownload().string() + "/" + filename;
        // 打开文件
        // gidle
        file::showFileInFolder(std::filesystem::u8path(temp_path));
        // 取消选中
    }

    static gboolean updateDownloadProgress(gpointer user_data) {
        progressMsg *msg = (progressMsg *) user_data;
        auto &pair = download_progressbars[msg->id];
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pair.second), msg->fraction);
        free(msg);
        return FALSE;
    }

    static gboolean updateDownloadFinish(gpointer user_data) {
        finishMsg *msg = (finishMsg *) user_data;
        auto &pair = download_progressbars[msg->id];
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pair.second), (msg->success ? 1.0 : 0.0));
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(pair.second),
                                  (pair.first +
                                   (msg->success ? "|下载成功" : "|下载失败")).c_str());
        showDialog("Download", "文件: " + pair.first + (msg->success ? "下载成功" : "下载失败"));
//        // remove
//        gtk_widget_unparent(pair.second);
//        download_progressbars.erase(msg->id);
        // 添加到队列
        terminated_map[pair.second] = *msg;
        free(msg);
        return FALSE;
    }


// 下载进度页面
    GtkWidget *UI::createDownloadPage() {
        // 创建主容器
        GtkWidget *downloadPage = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

        // vbox
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_widget_set_vexpand(vbox, TRUE);
        gtk_widget_set_hexpand(vbox, TRUE);
        gtk_widget_set_size_request(vbox, 400, 300);
        gtk_widget_set_margin_start(vbox, 5);
        gtk_widget_set_margin_end(vbox, 5);
        gtk_widget_set_margin_top(vbox, 5);
        gtk_widget_set_margin_bottom(vbox, 5);

        gtk_box_append(GTK_BOX(downloadPage), vbox);

        // label
        GtkWidget *label = gtk_label_new("下载进度");
        gtk_box_append(GTK_BOX(vbox), label);

        // 创建一个滚动窗口和列表用于显示下载进度
        GtkWidget *scrolled = gtk_scrolled_window_new();
        gtk_widget_set_size_request(scrolled, 400, 300);
        gtk_widget_set_vexpand(scrolled, TRUE);
        gtk_box_append(GTK_BOX(vbox), scrolled);

        // 创建一个列表
        download_list = gtk_list_box_new();
        gtk_list_box_set_selection_mode(GTK_LIST_BOX(download_list), GTK_SELECTION_NONE);
        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), download_list);

        // 选中下载项的事件
        g_signal_connect(download_list, "row-activated", G_CALLBACK(showFile), NULL);

        // 清理已完成项目
        GtkWidget *clearButton = gtk_button_new_with_label("清理结束项");
        gtk_widget_set_margin_bottom(clearButton, 20);
        gtk_widget_set_margin_start(clearButton, 5);
        gtk_widget_set_margin_end(clearButton, 5);
        gtk_box_append(GTK_BOX(vbox), clearButton);
        g_signal_connect(clearButton, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer user_data) {
            // 清理已完成项目
            for (auto &df: terminated_map) {
                gtk_widget_unparent(df.first);
                download_progressbars.erase(df.second.id);
            }
            terminated_map.clear();
        }), NULL);

        return downloadPage;
    }

// 对应的静态变量
    static std::vector<boost::asio::ip::udp::endpoint> neighbour_list;
    static GtkWidget *ip_label, *tcp_port_label, *udp_port_label, *download_dir_label, *shared_dir_label;

// 配置页面
    GtkWidget *UI::createConfigPage() {
        // 创建主容器
        GtkWidget *configPage = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

        // 暂时只能显示内容 不能修改
        GtkWidget *configLabel = gtk_label_new("配置信息 (请编辑config.ini后重启以应用配置)");
        gtk_box_append(GTK_BOX(configPage), configLabel);

        // 创建一个HBox容器
        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_box_append(GTK_BOX(configPage), hbox);

        // 创建一个滚动窗口和列表用于显示Peer列表
        GtkWidget *scrolled = gtk_scrolled_window_new();
        gtk_widget_set_size_request(scrolled, 400, 300);
        gtk_widget_set_vexpand(scrolled, TRUE);
        gtk_widget_set_hexpand(scrolled, TRUE);
        // margin
        gtk_widget_set_margin_start(scrolled, 5);
        gtk_widget_set_margin_end(scrolled, 5);
        gtk_widget_set_margin_top(scrolled, 5);
        gtk_widget_set_margin_bottom(scrolled, 5);
        gtk_box_append(GTK_BOX(hbox), scrolled);

        // 右边是一个VBox存放具体信息的Label
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_widget_set_size_request(vbox, 400, 300);
        gtk_widget_set_vexpand(vbox, TRUE);
        gtk_widget_set_hexpand(vbox, TRUE);
        // margin
        gtk_widget_set_margin_start(vbox, 5);
        gtk_widget_set_margin_end(vbox, 5);
        gtk_widget_set_margin_top(vbox, 5);
        gtk_widget_set_margin_bottom(vbox, 5);
        gtk_box_append(GTK_BOX(hbox), vbox);

        // label
        GtkWidget *config_label = gtk_label_new("具体信息");
        gtk_box_append(GTK_BOX(vbox), config_label);
        // ip
        ip_label = gtk_label_new("");
        gtk_box_append(GTK_BOX(vbox), ip_label);
        // udp port
        udp_port_label = gtk_label_new("");
        gtk_box_append(GTK_BOX(vbox), udp_port_label);
        // tcp port
        tcp_port_label = gtk_label_new("");
        gtk_box_append(GTK_BOX(vbox), tcp_port_label);
        // download path
        download_dir_label = gtk_label_new("");
        gtk_box_append(GTK_BOX(vbox), download_dir_label);
        // upload path
        shared_dir_label = gtk_label_new("");
        gtk_box_append(GTK_BOX(vbox), shared_dir_label);


        GtkWidget *peer_list = gtk_list_box_new();
        peer = getPeerInfo();
        // 选择事件
        g_signal_connect (peer_list, "row-selected", G_CALLBACK(+[](GtkListBox *self,
                                                                    GtkListBoxRow *row,
                                                                    gpointer user_data) {
                                                                    // 获取选中的行
                                                                    // 获取选中行的索引
                                                                    int index = gtk_list_box_row_get_index(row);

                                                                    if (index == 0) {
                                                                        // 是自己的信息
                                                                        gtk_label_set_text(GTK_LABEL(ip_label), std::string("IP: " + peer.getAddress().to_string()).c_str());
                                                                        gtk_label_set_text(GTK_LABEL(tcp_port_label),
                                                                                           std::string("TCP Port: " + std::to_string(peer.getTCPPort())).c_str());
                                                                        gtk_label_set_text(GTK_LABEL(udp_port_label),
                                                                                           std::string("UDP Port: " + std::to_string(peer.getUDPPort())).c_str());
                                                                        gtk_label_set_text(GTK_LABEL(download_dir_label),
                                                                                           std::string("Download Path: " + peer.getDownload().string()).c_str());
                                                                        gtk_label_set_text(GTK_LABEL(shared_dir_label),
                                                                                           std::string("Shared Path: " + peer.getRoot().string()).c_str());
                                                                        return;
                                                                    }

                                                                    // 是邻居的信息
                                                                    boost::asio::ip::udp::endpoint neighbour = neighbour_list[index - 1];
                                                                    gtk_label_set_text(GTK_LABEL(ip_label), std::string("IP: " + neighbour.address().to_string()).c_str());
                                                                    gtk_label_set_text(GTK_LABEL(udp_port_label),
                                                                                       std::string("UDP Port: " + std::to_string(neighbour.port())).c_str());
                                                                    // 其它Label不显示
                                                                    gtk_label_set_text(GTK_LABEL(tcp_port_label), "");
                                                                    gtk_label_set_text(GTK_LABEL(download_dir_label), "");
                                                                    gtk_label_set_text(GTK_LABEL(shared_dir_label), "");
                                                                }

        ), NULL);

        // 添加一些列表项
        GtkWidget *item;
        item = gtk_label_new("Peer");
        gtk_list_box_insert(GTK_LIST_BOX (peer_list), item, -1);
        int i = 0;
        for (auto neighbour: peer.getNeighbors()) {
            item = gtk_label_new(std::string("Neighbor " + std::to_string(i++)).c_str());
            gtk_list_box_insert(GTK_LIST_BOX (peer_list), item, -1);
            neighbour_list.push_back(neighbour.first);
        }

        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), peer_list);


        return
                configPage;
    }

// 系统状态页面 暂时不实现更多功能

    GtkWidget *UI::createStatusPage() {
        // 创建主容器
        GtkWidget *statusPage = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

        // 操作说明
        // 标题
        GtkWidget *title = gtk_label_new("操作说明");
        gtk_box_append(GTK_BOX(statusPage), title);
        // 大型文本框
        GtkWidget *scrolled = gtk_scrolled_window_new();
        gtk_widget_set_size_request(scrolled, 400, 300);
        gtk_widget_set_vexpand(scrolled, TRUE);
        gtk_widget_set_hexpand(scrolled, TRUE);
        // margin
        gtk_widget_set_margin_start(scrolled, 5);
        gtk_widget_set_margin_end(scrolled, 5);
        gtk_widget_set_margin_top(scrolled, 5);
        gtk_widget_set_margin_bottom(scrolled, 5);
        gtk_box_append(GTK_BOX(statusPage), scrolled);
        // 文本框
        GtkWidget *text_view = gtk_text_view_new();
        gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
        gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), text_view);
        // 文本缓冲区
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
        // 文本迭代器
        GtkTextIter iter;
        gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);
        // 插入文本
        gtk_text_buffer_insert(buffer, &iter, "欢迎使用本洪泛文件分享查询器。 作者: 【数据删除】 SCU 2021级本科生\n", -1);
        gtk_text_buffer_insert(buffer, &iter, "程序主要由查询、下载、配置三个选项页构成。\n", -1);
        gtk_text_buffer_insert(buffer, &iter, "在启动程序之前，务必先配置好程序目录下的config.ini：\n", -1);
        gtk_text_buffer_insert(buffer, &iter, "    [Peer]\n", -1);
        gtk_text_buffer_insert(buffer, &iter, "    IP = 你的IP地址\n", -1);
        gtk_text_buffer_insert(buffer, &iter, "    TCP_Port = 你的TCP端口号\n", -1);
        gtk_text_buffer_insert(buffer, &iter, "    UDP_Port = 你的UDP端口号\n", -1);
        gtk_text_buffer_insert(buffer, &iter, "    Download_Dir = 你的下载目录\n", -1);
        gtk_text_buffer_insert(buffer, &iter, "    Shared_Dir = 你的共享目录\n", -1);
        gtk_text_buffer_insert(buffer, &iter, "    [Neighbour1]\n", -1);
        gtk_text_buffer_insert(buffer, &iter, "    IP = 邻居的IP地址\n", -1);
        gtk_text_buffer_insert(buffer, &iter, "    UDP_Port = 邻居的UDP端口号\n", -1);
        gtk_text_buffer_insert(buffer, &iter, "    [Neighbour2...] 依此类推\n", -1);
        gtk_text_buffer_insert(buffer, &iter, "    ...\n", -1);
        gtk_text_buffer_insert(buffer, &iter, "    [NeighbourN]\n", -1);
        gtk_text_buffer_insert(buffer, &iter, "在查询页，你可以在顶部搜索框输入查询文件名/文件夹名称的前缀名称。\n", -1);
        gtk_text_buffer_insert(buffer, &iter, "点击查询按钮，等待2s即可获取查询结果。\n", -1);
        gtk_text_buffer_insert(buffer, &iter, "中间两个列表，左侧会显示资源节点的地址和端口信息。\n", -1);
        gtk_text_buffer_insert(buffer, &iter, "右侧会显示该资源节点所共享的文件列表。\n", -1);
        gtk_text_buffer_insert(buffer, &iter, "选择你想要的文件，点击下方下载按钮，下载任务会被自动添加。\n", -1);
        // 分隔符
        gtk_text_buffer_insert(buffer, &iter, "----------------------------------------\n", -1);
        gtk_text_buffer_insert(buffer, &iter, "在下载页，你可以看到当前正在下载的任务列表，和其它状态（完成、失效）的下载任务。\n", -1);
        gtk_text_buffer_insert(buffer, &iter, "你可以单击完成项进入目标目录。\n", -1);
        gtk_text_buffer_insert(buffer, &iter, "单击下方按钮清除多余下载项。\n", -1);
        // 分隔符
        gtk_text_buffer_insert(buffer, &iter, "----------------------------------------\n", -1);
        gtk_text_buffer_insert(buffer, &iter, "在配置页，你可以查看配置文件信息。左侧列表选择peer，右侧是具体配置。\n", -1);

//        // 创建显示系统状态信息的标签
//        status_label = gtk_label_new("系统状态信息");
//        gtk_box_append(GTK_BOX(statusPage), status_label);
//
//        // 下载任务数量
//        down_count_label = gtk_label_new("下载任务总计: 0");
//        gtk_box_append(GTK_BOX(statusPage), down_count_label);
//
//        // 正在服务的数量
//        serving_count_label = gtk_label_new("正在服务下载请求数: 0");
//        gtk_box_append(GTK_BOX(statusPage), serving_count_label);
//
//        // 转发的查询数量
//        query_count_label = gtk_label_new("转发的查询数量: 0");
//        gtk_box_append(GTK_BOX(statusPage), query_count_label);

        return statusPage;
    }

    void UI::setPeerInfoGetter(std::function<Peer()> getter) {
        getPeerInfo = getter;
    }

    void UI::setDownloadCountGetter(std::function<int()> getter) {
        getDownloadCount = getter;
    }

    void UI::setServingCountGetter(std::function<int()> getter) {
        getServingCount = getter;
    }

    void UI::setQuerySender(std::function<void(std::string)> callback) {
        sendQuery = callback;
    }

    void UI::setDownloadCallback(std::function<void(const QueryHit &query_hit, const int query_hit_index)> callback) {
        download = callback;
    }

    void UI::updateDownloadProgressCallback(const int down_id, const double progress) {
        g_idle_add(GSourceFunc(updateDownloadProgress), new progressMsg{down_id, progress});
    }

    void UI::downloadSuccessCallback(const int down_id, bool success) {
        finishMsg msg = {down_id, success};
        g_idle_add(GSourceFunc(updateDownloadFinish), new finishMsg{down_id, success});
    }

}