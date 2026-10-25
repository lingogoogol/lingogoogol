import std;
import lgo.io;
import storage.google;
import storage.kernel;
int main() {
    try {
        lgo::init_logfile("logfile/folder-prepare.txt");
        download_task_t task{};
        task.m_prepared=std::async(std::launch::async,[&] {
            folder_download(&google_easy_t::get(),&task,"folder-main","output/nested");
        });
        task.m_prepared.get();
        if(task.m_file_arr.size()!=4 || task.m_size!=20)
            throw std::runtime_error("Recursive/paginated folder entries differ.");
        if(!std::filesystem::is_directory("output/nested/empty-dir"))
            throw std::runtime_error("Empty directory was not prepared.");
        std::println("PASS: recursive folder preparation preserves paginated JSON listings");
        return 0;
    } catch(const std::exception& e) {std::println("FAIL: {}",e.what());return 1;}
}
