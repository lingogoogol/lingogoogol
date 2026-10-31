export module storage.google.cloud;
import std;
import storage.google.oauth;
import storage.state;

export {
    // Preparing the project precedes the name dialog; creating the account/key
    // is a separate step so dismissing that dialog never creates a private key.
    auto google_cloud_prepare(const std::shared_ptr<google_oauth_user_t>& user,
                              const std::shared_ptr<request_control_t>& control) -> std::string;
    auto google_cloud_validate_account(const std::string& name) -> void;
    auto google_cloud_create_account(const std::shared_ptr<google_oauth_user_t>& user,
                                    const std::shared_ptr<request_control_t>& control,
                                    const std::string& project, const std::string& name,
                                    const std::filesystem::path& directory = "key") -> std::filesystem::path;
}
