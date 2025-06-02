#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <ctime>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

// 模仿C++17 filesystem 的基础实现
namespace fs {
    bool exists(const std::string& path) {
        struct stat buffer;
        return stat(path.c_str(), &buffer) == 0;
    }

    bool create_directory(const std::string& path) {
        return mkdir(path.c_str(), 0777) == 0;
    }

    std::vector<std::string> directory_iterator(const std::string& path) {
        std::vector<std::string> files;
        DIR* dir = opendir(path.c_str());
        if (dir) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != NULL) {
                if (entry->d_name[0] != '.') {
                    files.push_back(path + "/" + entry->d_name);
                }
            }
            closedir(dir);
        }
        return files;
    }
}

// 情感状态
enum Emotion {
    CALM,
    HAPPY,
    SAD,
    ANGRY,
    CURIOUS,
    NUM_EMOTIONS
};

// 自定义字符串处理函数
std::string to_lower(const std::string& str) {
    std::string lower_str = str;
    for (size_t i = 0; i < lower_str.size(); ++i) {
        lower_str[i] = std::tolower(lower_str[i]);
    }
    return lower_str;}

// 简化版随机数生成器
class SimpleRNG {
    unsigned int seed;
public:
    SimpleRNG() : seed(time(NULL)) {}
    
    void set_seed(unsigned int s) { seed = s; }
    
    double rand_double() {
        seed = seed * 1103515245 + 12345;
        return static_cast<double>(seed) / UINT_MAX;
    }
    
    int rand_int(int min, int max) {
        seed = seed * 1103515245 + 12345;
        return min + (seed % (max - min + 1));
    }
};

// 语言模型核心类
class LanguageModel {
private:
    typedef std::map<std::string, std::map<std::string, double> > TransitionMap;
    typedef std::map<std::string, std::map<std::string, int> > FrequencyMap;
    
    TransitionMap transitions;
    FrequencyMap frequencies;
    std::map<std::string, int> word_freq;
    
    std::map<Emotion, double> emotion_weights;
    Emotion current_emotion;
    
    int current_block;
    static const int BLOCK_SIZE = 5000;
    std::string model_dir;
    
    SimpleRNG rng;

public:
    LanguageModel() : current_block(0), model_dir("model_blocks/") {
        emotion_weights[CALM] = 0.3;
        emotion_weights[HAPPY] = 0.6;
        emotion_weights[SAD] = 0.4;
        emotion_weights[ANGRY] = 0.2;
        emotion_weights[CURIOUS] = 0.8;
        current_emotion = CALM;
        
        // 创建模型目录
        if (!fs::exists(model_dir)) {
            fs::create_directory(model_dir);
        }
    }
    
    // 学习新文本
    void learn(const std::string& text) {
        std::vector<std::string> words;
        std::string word;
        
        // 分词处理
        for (size_t i = 0; i < text.size(); ++i) {
            char c = text[i];
            if (std::isspace(c) || c == '.' || c == ',' || c == '!' || c == '?') {
                if (!word.empty()) {
                    words.push_back(to_lower(word));
                    word.clear();
                }
                continue;
            }
            word += c;
        }
        if (!word.empty()) words.push_back(to_lower(word));
        
        // 更新模型
        for (size_t i = 0; i < words.size(); ++i) {
            const std::string& current = words[i];
            word_freq[current]++;
            
            // 如果当前字数超过块大小，切换到下一个块
            if (word_freq.size() % BLOCK_SIZE == 0) {
                save_current_block();
                current_block++;
                transitions.clear();
                frequencies.clear();
            }
            
            // 处理与前一个词的关联
            if (i > 0) {
                const std::string& prev = words[i-1];
                
                // 更新频率
                frequencies[prev][current]++;
                
                // 更新权重
                if (transitions[prev].find(current) == transitions[prev].end()) {
                    transitions[prev][current] = 1.0;
                } else {
                    // 基于情绪和频率调整权重
                    transitions[prev][current] += emotion_weights[current_emotion] * 0.1;
                }
            }
        }
    }
    
    // 生成文本
    std::string generate(int max_length = 15) {
        // 随机选择起始词（基于频率）
        if (word_freq.empty()) {
            return "需要更多数据...";
        }
        
        // 创建候选词列表
        std::vector<std::string> words;
        std::vector<double> probs;
        
        for (std::map<std::string, int>::const_iterator it = word_freq.begin();
             it != word_freq.end(); ++it) {
            words.push_back(it->first);
            probs.push_back(it->second);
        }
        
        // 选择起始词
        int start_idx = weighted_choice(probs);
        std::string current_word = words[start_idx];
        std::string result = current_word;
        
        // 基于权重生成连续文本
        for (int i = 0; i < max_length - 1; ++i) {
            if (transitions.find(current_word) == transitions.end() ||
                transitions[current_word].empty()) {
                break; // 没有后续词
            }
            
            // 准备候选词列表
            words.clear();
            probs.clear();
            const std::map<std::string, double>& next_words = transitions[current_word];
            
            for (std::map<std::string, double>::const_iterator it = next_words.begin();
                 it != next_words.end(); ++it) {
                words.push_back(it->first);
                probs.push_back(it->second);
            }
            
            // 选择下一个词
            int next_idx = weighted_choice(probs);
            std::string next_word = words[next_idx];
            
            // 添加适当的标点
            if (i % 4 == 0 && next_word.size() > 3) {
                result += ". " + next_word;
            } else {
                result += " " + next_word;
            }
            
            current_word = next_word;
        }
        
        // 确保句子以标点结束
        if (result.back() != '.' && result.back() != '!' && result.back() != '?') {
            result += ".";
        }
        
        return result;
    }
    
    // 保存当前块
    void save_current_block() {
        std::string block_file = model_dir + "block_" + to_string(current_block) + ".dat";
        std::ofstream out_file(block_file.c_str());
        
        for (TransitionMap::const_iterator it_prev = transitions.begin();
             it_prev != transitions.end(); ++it_prev) {
            
            const std::string& prev_word = it_prev->first;
            const std::map<std::string, double>& next_map = it_prev->second;
            
            for (std::map<std::string, double>::const_iterator it_next = next_map.begin();
                 it_next != next_map.end(); ++it_next) {
                
                const std::string& next_word = it_next->first;
                double weight = it_next->second;
                int freq = frequencies[prev_word][next_word];
                
                out_file << prev_word << " " << next_word << " " 
                         << freq << " " << weight << "\n";
            }
        }
    }
    
    // 加载指定块
    void load_block(int block_number) {
        std::string block_file = model_dir + "block_" + to_string(block_number) + ".dat";
        current_block = block_number;
        
        std::ifstream in_file(block_file.c_str());
        if (!in_file) return;
        
        std::string prev_word, next_word;
        int freq;
        double weight;
        
        while (in_file >> prev_word >> next_word >> freq >> weight) {
            frequencies[prev_word][next_word] = freq;
            transitions[prev_word][next_word] = weight;
            word_freq[prev_word] = std::max(word_freq[prev_word], freq);
        }
    }

private:
    // 基础类型转换
    std::string to_string(int value) {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }
    
    // 加权随机选择
    int weighted_choice(const std::vector<double>& weights) {
        double total = 0.0;
        for (size_t i = 0; i < weights.size(); ++i) {
            total += weights[i];
        }
        
        double r = rng.rand_double() * total;
        double sum = 0.0;
        
        for (size_t i = 0; i < weights.size(); ++i) {
            sum += weights[i];
            if (r <= sum) return i;
        }
        
        return weights.size() - 1;
    }
};

// 主函数 - 简化的对话循环
int main() {
    LanguageModel model;
    model.load_block(0); // 尝试加载基础块
    
    std::cout << "简易AI系统已启动 (输入 'exit' 退出)\n";
    
    while (true) {
        std::cout << "你: ";
        std::string input;
        std::getline(std::cin, input);
        
        if (input == "exit") {
            model.save_current_block();
            break;
        }
        
        // 学习用户输入
        model.learn(input);
        
        // 生成响应
        std::string response = model.generate();
        std::cout << "AI: " << response << std::endl;
    }
    
    std::cout << "模型已保存，程序结束。\n";
    return 0;
}//by DeepSeek for 5hour.
