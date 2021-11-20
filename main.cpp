#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define RAPIDJSON_HAS_STDSTRING true
#include <vector>
#include <iostream>
#include <Windows.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <SFML/Graphics.hpp>
#include "resource.h"

sf::Font *font = nullptr;
sf::Texture *texture = nullptr;

volatile bool pressed = false;
#ifdef DEBUG
volatile uint16_t reallocCount = 0;
#endif

void error(std::wstring error) {
  MessageBoxW(NULL, error.c_str(), L"Progress error!", MB_ICONERROR | MB_OK);
  abort();
}

class Tree {
  //Render
  sf::VertexBuffer bar_;
  sf::VertexBuffer buttons_;
  sf::VertexBuffer percent_;
  sf::Vertex *vtBar_ = nullptr;
  sf::Vertex *vtButton_ = nullptr;
  sf::Vertex *vtPercent_ = nullptr;
  sf::Text name_;

  //Button
  sf::IntRect barRect_;
  sf::IntRect *buttonsRects_ = nullptr;

  //Container
  std::vector<Tree> tree_;

  //Mode
  sf::Vector2u position_;
  bool isFolder_ = false;

  //State
  bool isRoot_ = false;
  bool isVisible_ = false;
  bool isRenamed_ = false;
  bool isProperty_ = false;
  bool isChanged_ = false;
  bool isDeleted_ = false;
  bool isDone_ = false;
  uint8_t percentValue_ = 0;
public:
  Tree() {
#ifdef DEBUG
    std::wcout << L"Tree():  Constructor" << std::endl;
    reallocCount++;
#endif // DEBUG
    init();
  }

  //Deleting copy constructor
  Tree(Tree &tree) = delete;
  Tree(const Tree &tree) = delete;

  //Move constructor
  Tree(Tree &&tree) noexcept {
#ifdef DEBUG
    std::wcout << L"Tree():  Move constructor" << std::endl;
    reallocCount++;
#endif // DEBUG
    init();
    isFolder_ = tree.isFolder_;
    isRoot_ = tree.isRoot_;
    isVisible_ = tree.isVisible_;
    isRenamed_ = tree.isRenamed_;
    isProperty_ = tree.isProperty_;
    isChanged_ = tree.isChanged_;
    isDeleted_ = tree.isDeleted_;
    isDone_ = tree.isDone_;
    name_ = tree.name_;
    std::swap(position_, tree.position_);
    std::swap(vtBar_, tree.vtBar_);
    std::swap(vtButton_, tree.vtButton_);
    std::swap(vtPercent_, tree.vtPercent_);
    std::swap(buttonsRects_, tree.buttonsRects_);
    tree_.swap(tree.tree_);
    bar_.swap(tree.bar_);
    buttons_.swap(tree.buttons_);
    percent_.swap(tree.percent_);
  }

  //Deleting cope operators
  Tree &operator=(Tree &tree) = delete;
  Tree &operator=(const Tree &tree) = delete;

  //Move operators
  Tree &operator=(Tree &&tree) noexcept {
#ifdef DEBUG
    std::wcout << L"Tree():  Move operator" << std::endl;
    reallocCount++;
#endif // DEBUG
    if(this == &tree) {
      return *this;
    }

    if(vtBar_) {
      delete[] vtBar_;
      vtBar_ = nullptr;
    }
    if(vtButton_) {
      delete[] vtButton_;
      vtButton_ = nullptr;
    }
    if(vtPercent_) {
      delete[] vtPercent_;
      vtPercent_ = nullptr;
    }
    if(buttonsRects_) {
      delete[] buttonsRects_;
      buttonsRects_ = nullptr;
    }

    init();
    isFolder_ = tree.isFolder_;
    isRoot_ = tree.isRoot_;
    isVisible_ = tree.isVisible_;
    isRenamed_ = tree.isRenamed_;
    isProperty_ = tree.isProperty_;
    isChanged_ = tree.isChanged_;
    isDeleted_ = tree.isDeleted_;
    isDone_ = tree.isDone_;
    name_ = tree.name_;
    tree_.swap(tree.tree_);
    std::swap(position_, tree.position_);
    std::swap(vtBar_, tree.vtBar_);
    std::swap(vtButton_, tree.vtButton_);
    std::swap(vtPercent_, tree.vtPercent_);
    std::swap(buttonsRects_, tree.buttonsRects_);
    bar_.swap(tree.bar_);
    buttons_.swap(tree.buttons_);
    percent_.swap(tree.percent_);

    return *this;
  }

  ~Tree() {
#ifdef DEBUG
    std::wcout << L"~Tree(): Destructor" << std::endl;
    reallocCount++;
#endif // DEBUG
    if(vtBar_) {
      delete[] vtBar_;
      vtBar_ = nullptr;
    }
    if(vtButton_) {
      delete[] vtButton_;
      vtButton_ = nullptr;
    }
    if(vtPercent_) {
      delete[] vtPercent_;
      vtPercent_ = nullptr;
    }
    if(buttonsRects_) {
      delete[] buttonsRects_;
      buttonsRects_ = nullptr;
    }
  }

private:
  void init() {
    name_.setFont(*font);
    name_.setString("Unnamed");
    name_.setCharacterSize(16);

    //Setup VBO for bar
    bar_.setPrimitiveType(sf::Quads);
    bar_.setUsage(sf::VertexBuffer::Stream);
    bar_.create(8);

    //Setup VBO for buttons
    buttons_.setPrimitiveType(sf::Quads);
    buttons_.setUsage(sf::VertexBuffer::Stream);

    //Setup VBO for persent inducator
    percent_.setPrimitiveType(sf::Quads);
    percent_.setUsage(sf::VertexBuffer::Stream);
  }
public:

  void load(rapidjson::GenericValue<rapidjson::UTF16LE<>> &reader) {
    if(!reader.IsObject()) {
      error(L"Not an object");
    }
    if(!isRoot_) {
      if(!reader.HasMember(L"type")) {
        error(L"No member \"type\"");
      }
      if(!reader[L"type"].IsBool()) {
        error(L"Member \"type\" not a bool");
      }
      isFolder_ = reader[L"type"].GetBool();
    }
    if(!reader.HasMember(L"name")) {
      error(L"No member \"name\"");
    }
    if(!reader[L"name"].IsString()) {
      error(L"Member \"name\" not a string");
    }
    name_.setString(reader[L"name"].GetString());
    if(!reader.HasMember(L"data")) {
      error(L"No member \"data\"");
    }
    if(isFolder_) {
      if(!reader[L"data"].IsArray()) {
        error(L"Member \"data\" not an array");
      }
      rapidjson::SizeType size = reader[L"data"].Size();
      if(size == 0) {
        tree_.reserve(8);
      }
      else {
        tree_.reserve(size);
      }
      for(rapidjson::SizeType i = 0; i < size; i++) {
        tree_.emplace_back();
        tree_.back().load(reader[L"data"][i]);
        tree_.back().setVisible(true);
      }
    }
    else {
      if(!reader[L"data"].IsBool()) {
        error(L"Member \"data\" not a bool");
      }
      isDone_ = reader[L"data"].GetBool();
    }
  }

  void save(rapidjson::Writer<rapidjson::EncodedOutputStream<rapidjson::UTF16LE<>, rapidjson::FileWriteStream>, rapidjson::UTF16LE<>, rapidjson::UTF16LE<>> &writer) {
    writer.Key(L"type");
    writer.Bool(isFolder_);
    writer.Key(L"name");
    writer.String(name_.getString().toWideString());
    if(isFolder_) {
      writer.Key(L"data");
      writer.StartArray();
      for(std::size_t i = 0; i < tree_.size(); i++) {
        writer.StartObject();
        tree_[i].save(writer);
        writer.EndObject();
      }
      writer.EndArray();
    }
    else {
      writer.Key(L"data");
      writer.Bool(isDone_);
    }
  }

  inline void setRoot() {
    isRoot_ = true;
    isFolder_ = true;
    isVisible_ = true;
  }

  inline void setVisible(bool isVisible) {
    if(isRoot_) {
      throw std::logic_error("is a tree root");
    }
    isVisible_ = isVisible;
  }

  inline void setIsFolder(bool isFolder) {
    if(isRoot_) {
      throw std::logic_error("is a tree root");
    }
    isFolder_ = isFolder;
  }

  void setPosition(sf::Vector2u position) {
    position_ = position;

    name_.setPosition(sf::Vector2f(position_) + sf::Vector2f(2, 0));

    if(!vtBar_) {
      vtBar_ = new sf::Vertex[8];
    }
    if(isVisible_) {
      vtBar_[0].color = sf::Color(64, 64, 255);
      vtBar_[1].color = sf::Color(64, 64, 255);
      vtBar_[2].color = sf::Color(64, 64, 255);
      vtBar_[3].color = sf::Color(64, 64, 255);
    }
    else {
      vtBar_[0].color = sf::Color(128, 128, 255);
      vtBar_[1].color = sf::Color(128, 128, 255);
      vtBar_[2].color = sf::Color(128, 128, 255);
      vtBar_[3].color = sf::Color(128, 128, 255);
    }

    for(uint8_t i = 4; i < 8; i++) {
      vtBar_[i].color = isFolder_ ? sf::Color(160, 0, 0) : sf::Color(0, 160, 0);
    }
    //Main bar
    vtBar_[0].position.x = static_cast<float>(position_.x);
    vtBar_[0].position.y = static_cast<float>(position_.y);

    vtBar_[1].position.x = static_cast<float>(position_.x);
    vtBar_[1].position.y = static_cast<float>(position_.y) + 20;

    vtBar_[2].position.x = static_cast<float>(position_.x) + 400;
    vtBar_[2].position.y = static_cast<float>(position_.y) + 20;

    vtBar_[3].position.x = static_cast<float>(position_.x) + 400;
    vtBar_[3].position.y = static_cast<float>(position_.y);

    barRect_.left = position_.x;
    barRect_.top = position_.y;
    barRect_.width = 400;
    barRect_.height = 20;

    //Button render compute
    buttons_.create(isFolder_ ? 20 : 12);
    if(!vtButton_) {
      vtButton_ = new sf::Vertex[isFolder_ ? 20 : 12];
    }
    if(!buttonsRects_) {
      buttonsRects_ = new sf::IntRect[isFolder_ ? 5 : 3];
    }

    uint8_t index;
    for(uint8_t i = 0; i < (isFolder_ ? 5 : 3); i++) {
      index = i * 4;
      vtButton_[index + 0].color = sf::Color::White;
      vtButton_[index + 1].color = sf::Color::White;
      vtButton_[index + 2].color = sf::Color::White;
      vtButton_[index + 3].color = sf::Color::White;

      vtButton_[index + 0].position.x = static_cast<float>(position_.x) + 410.0F + static_cast<float>(i) * 30.0F;
      vtButton_[index + 0].position.y = static_cast<float>(position_.y);

      vtButton_[index + 1].position.x = static_cast<float>(position_.x) + 410.0F + static_cast<float>(i) * 30.0F;
      vtButton_[index + 1].position.y = static_cast<float>(position_.y) + 20.0F;

      vtButton_[index + 2].position.x = static_cast<float>(position_.x) + 430.0F + static_cast<float>(i) * 30.0F;
      vtButton_[index + 2].position.y = static_cast<float>(position_.y) + 20.0F;

      vtButton_[index + 3].position.x = static_cast<float>(position_.x) + 430.0F + static_cast<float>(i) * 30.0F;
      vtButton_[index + 3].position.y = static_cast<float>(position_.y);

      vtButton_[index + 0].texCoords = sf::Vector2f(0.0F + i * 20.0F, isFolder_ ? 0.0F : 20.0F);
      vtButton_[index + 1].texCoords = sf::Vector2f(0.0F + i * 20.0F, isFolder_ ? 20.0F : 40.0F);
      vtButton_[index + 2].texCoords = sf::Vector2f(20.0F + i * 20.0F, isFolder_ ? 20.0F : 40.0F);
      vtButton_[index + 3].texCoords = sf::Vector2f(20.0F + i * 20.0F, isFolder_ ? 0.0F : 20.0F);

      buttonsRects_[i] = sf::IntRect(position_.x + 410 + i * 30, position_.y, 20, 20);
    }
    buttons_.update(vtButton_);
    for(std::size_t i = 0; i < tree_.size(); i++) {
      tree_[i].setPosition(sf::Vector2u(position_.x + 10, i == 0 ? position_.y + 30 : tree_[i - 1].getPosition().y + (tree_[i - 1].getHeight() + 1) * 30));
    }
    percentUpdate();
  }

  inline bool getChanged() {
    return isChanged_;
  }

  inline bool getDeleted() {
    return isDeleted_;
  }

  inline bool getIsFolder() {
    return isFolder_;
  }

  inline bool getCheckValue() {
    return isDone_;
  }

  inline sf::Vector2u getPosition() {
    return position_;
  }

  std::size_t getHeight() {
    if(!isVisible_) {
      return 0;
    }
    std::size_t out = tree_.size();
    for(auto &i : tree_) {
      out += i.getHeight();
    }
    return out;
  }

  inline uint8_t getPercent() {
    return percentValue_;
  }

  void percentUpdate() {
    float percent = isDone_ ? 100.0 : 0.0;
    if(isFolder_) {
      float x = 0.0;
      percent = 100.0 / static_cast<float>(tree_.size());
      for(auto &i : tree_) {
        if(i.getIsFolder()) {
          x += i.getPercent() / 100.0;
        }
        else {
          x += i.getCheckValue() ? 1.0 : 0.0;
        }
      }
      percent *= x;
      percentValue_ = static_cast<uint8_t>(percent);
      std::string percentString = std::to_string(percentValue_) + "%";
      uint8_t lenght = static_cast<uint8_t>(percentString.length());
      percent_.create((static_cast<std::size_t>(lenght)) * 4);
      if(vtPercent_) {
        delete[] vtPercent_;
      }
      vtPercent_ = new sf::Vertex[static_cast<std::size_t>(lenght) * 4];
      uint8_t index;
      for(uint8_t i = 0; i < lenght; i++) {
        index = i * 4;
        vtPercent_[index + 0].position.x = (float) position_.x + 380 - 20 * i;
        vtPercent_[index + 0].position.y = (float) position_.y;

        vtPercent_[index + 1].position.x = (float) position_.x + 380 - 20 * i;
        vtPercent_[index + 1].position.y = (float) position_.y + 20;

        vtPercent_[index + 2].position.x = (float) position_.x + 400 - 20 * i;
        vtPercent_[index + 2].position.y = (float) position_.y + 20;

        vtPercent_[index + 3].position.x = (float) position_.x + 400 - 20 * i;
        vtPercent_[index + 3].position.y = (float) position_.y;

        char ch = percentString[(static_cast<std::size_t>(lenght) - 1) - i];
        if(ch == '%') {
          vtPercent_[index + 0].texCoords = sf::Vector2f(180, 0);
          vtPercent_[index + 1].texCoords = sf::Vector2f(180, 20);
          vtPercent_[index + 2].texCoords = sf::Vector2f(200, 20);
          vtPercent_[index + 3].texCoords = sf::Vector2f(200, 0);
        }
        else {
          ch = atoi(percentString.substr((static_cast<std::size_t>(lenght) - 1) - i, 1).c_str());
          vtPercent_[index + 0].texCoords = sf::Vector2f((float) ch * 20, 40);
          vtPercent_[index + 1].texCoords = sf::Vector2f((float) ch * 20, 60);
          vtPercent_[index + 2].texCoords = sf::Vector2f((float) (ch + 1) * 20, 60);
          vtPercent_[index + 3].texCoords = sf::Vector2f((float) (ch + 1) * 20, 40);
        }
      }
    }
    percent_.update(vtPercent_);
    float progress = 396 * (percent / 100);
    vtBar_[4].position.x = position_.x + 2;
    vtBar_[4].position.y = position_.y + 2;

    vtBar_[5].position.x = position_.x + 2;
    vtBar_[5].position.y = position_.y + 20 - 2;

    vtBar_[6].position.x = position_.x + progress + 2;
    vtBar_[6].position.y = position_.y + 20 - 2;

    vtBar_[7].position.x = position_.x + progress + 2;
    vtBar_[7].position.y = position_.y + 2;
    bar_.update(vtBar_);
  }

  bool event(sf::Event &event, sf::RenderWindow &window) {
    isChanged_ = false;
    bool out = false;
    switch(event.type) {
      case sf::Event::MouseButtonPressed:
      {
        if((event.mouseButton.button != sf::Mouse::Left) || pressed) {
          break;
        }
        sf::Vector2i mousePos = sf::Vector2i(window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y)));
        if(mousePos.y < position_.y) {
          break;
        }
        if(barRect_.contains(mousePos)) {
          if(isFolder_) {
            isVisible_ = !isVisible_;
          }
          else {
            isDone_ = !isDone_;
          }
          isChanged_ = true;
          out = true;
        }
        if(buttonsRects_[0].contains(mousePos)) {
          isProperty_ = !isProperty_;
          out = true;
        }
        if(isProperty_) {
          if(isFolder_) {
            if(buttonsRects_[1].contains(mousePos)) {
              tree_.emplace_back();
              tree_.back().setIsFolder(false);
              tree_.back().setVisible(true);
              tree_.back().setPosition(sf::Vector2u(position_.x + 10, tree_.size() == 1 ? std::size_t(position_.y) + 30 : (tree_.end() - 2)->getPosition().y + ((tree_.end() - 2)->getHeight() + 1) * 30));
              percentUpdate();
              isChanged_ = true;
              out = true;
            }
            if(buttonsRects_[2].contains(mousePos)) {
              tree_.emplace_back();
              tree_.back().setIsFolder(true);
              tree_.back().setVisible(true);
              tree_.back().setPosition(sf::Vector2u(position_.x + 10, tree_.size() == 1 ? std::size_t(position_.y) + 30 : (tree_.end() - 2)->getPosition().y + ((tree_.end() - 2)->getHeight() + 1) * 30));
              percentUpdate();
              isChanged_ = true;
              out = true;
            }
            if(buttonsRects_[3].contains(mousePos)) {
              name_.setFillColor(sf::Color::Yellow);
              isRenamed_ = true;
              out = true;
            }
            if(buttonsRects_[4].contains(mousePos)) {
              isDeleted_ = true;
              out = true;
            }
          }
          else {
            if(buttonsRects_[1].contains(mousePos)) {
              name_.setFillColor(sf::Color::Yellow);
              isRenamed_ = true;
              out = true;
            }
            if(buttonsRects_[2].contains(mousePos)) {
              isDeleted_ = true;
              out = true;
            }
          }
        }
        pressed = out;
        break;
      }
      case sf::Event::MouseButtonReleased:
      {
        if(event.mouseButton.button == sf::Mouse::Left) {
          pressed = false;
        }
        break;
      }
      case sf::Event::TextEntered:
      {
        if(isRenamed_) {
          switch(event.text.unicode) {
            case 8:
            {
              std::wstring name = name_.getString();
              if(name.length() == 0) {
                break;
              }
              name.erase(name.end() - 1);
              name_.setString(name);
              out = true;
              break;
            }
            case 13:
            {
              name_.setFillColor(sf::Color::White);
              isRenamed_ = false;
              out = true;
              break;
            }
            default:
            {
              std::wstring name = name_.getString();
              if(name.length() > 30) {
                break;
              }
              name += event.text.unicode;
              name_.setString(name);
              out = true;
              break;
            }
          }
        }
        break;
      }
    }
    if(isVisible_ && isFolder_) {
      for(std::size_t i = 0; i < tree_.size(); i++) {
        out = tree_[i].event(event, window) || out;
        if(tree_[i].getDeleted()) {
          isChanged_ = true;
          tree_.erase(tree_.begin() + i);
          for(std::size_t j = i; j < tree_.size(); j++) {
            tree_[j].setPosition(sf::Vector2u(position_.x + 10, j == 0 ? std::size_t(position_.y) + 30 : tree_[j - 1].getPosition().y + (tree_[j - 1].getHeight() + 1) * 30));
          }
          percentUpdate();
          i--;
          continue;
        }
        if(tree_[i].getChanged()) {
          isChanged_ = true;
          for(std::size_t j = i; j < tree_.size(); j++) {
            tree_[j].setPosition(sf::Vector2u(position_.x + 10, j == 0 ? std::size_t(position_.y) + 30 : tree_[j - 1].getPosition().y + (tree_[j - 1].getHeight() + 1) * 30));
          }
          percentUpdate();
        }
      }
    }
    return out;
  }

  void draw(sf::RenderWindow &window) {
    if(isVisible_) {
      for(auto &i : tree_) {
        i.draw(window);
      }
    }
    window.draw(bar_);
    if(isProperty_) {
      window.draw(buttons_, texture);
    }
    else {
      window.draw(buttons_, 0, 4, texture);
    }
    if(isFolder_) {
      window.draw(percent_, texture);
    }
    window.draw(name_);
  }
};

void load(Tree &tree) {
  FILE *file;
  _wfopen_s(&file, L"progress.json", L"r");

  if(!file) {
    return;
  }

  char buff[1024];
  rapidjson::FileReadStream fileStream(file, buff, sizeof(buff));
  rapidjson::EncodedInputStream<rapidjson::UTF16LE<>, rapidjson::FileReadStream> encodedInputStream(fileStream);
  rapidjson::GenericDocument<rapidjson::UTF16LE<>> json;
  json.ParseStream(encodedInputStream);

  tree.load(json);

  fclose(file);
}

void save(Tree &tree) {
  FILE *file;
  _wfopen_s(&file, L"progress.json", L"w");

  if(!file) {
    error(L"Cannot open file for saving");
    return;
  }

  char buff[1024];
  rapidjson::FileWriteStream fileStream(file, buff, sizeof(buff));
  rapidjson::EncodedOutputStream<rapidjson::UTF16LE<>, rapidjson::FileWriteStream> encodedOutputStream(fileStream);
  rapidjson::Writer<rapidjson::EncodedOutputStream<rapidjson::UTF16LE<>, rapidjson::FileWriteStream>, rapidjson::UTF16LE<>, rapidjson::UTF16LE<>> writer(encodedOutputStream);

  writer.StartObject();
  tree.save(writer);
  writer.EndObject();

  fclose(file);
}

#ifdef DEBUG
void reallocStat() {
  if(reallocCount != 0) {
    std::wcout << L"Realloc total: " << reallocCount << std::endl;
    reallocCount = 0;
  }
}
#endif // DEBUG

int main() {
#ifndef DEBUG
  FreeConsole();
#endif // !DEBUG
  font = new sf::Font;
  font->loadFromFile("C:\\Windows\\Fonts\\arial.ttf");

  texture = new sf::Texture;

  HRSRC hResource = NULL;
  HGLOBAL hMemory = NULL;

  hResource = FindResourceW(NULL, MAKEINTRESOURCE(IDB_PNG1), L"PNG");
  if(!hResource) {
    error(L"Cannot load internal resource\n\nInternal integrity violation");
    return EXIT_FAILURE;
  }
  hMemory = LoadResource(NULL, hResource);
  if(!hMemory) {
    error(L"Cannot load internal resource\n\nInternal integrity violation");
    return EXIT_FAILURE;
  }
  sf::Image icon;
  icon.loadFromMemory(LockResource(hMemory), SizeofResource(NULL, hResource));

  hResource = FindResourceW(NULL, MAKEINTRESOURCE(IDB_PNG2), L"PNG");
  if(!hResource) {
    error(L"Cannot load internal resource\n\nInternal integrity violation");
    return EXIT_FAILURE;
  }
  hMemory = LoadResource(NULL, hResource);
  if(!hMemory) {
    error(L"Cannot load internal resource\n\nInternal integrity violation");
    return EXIT_FAILURE;
  }
  texture->loadFromMemory(LockResource(hMemory), SizeofResource(NULL, hResource));

  Tree tree;
  tree.setRoot();
  load(tree);
  tree.setPosition(sf::Vector2u(5, 5));
#ifdef DEBUG
  reallocStat();
#endif // DEBUG

  sf::View view;
  view.setCenter(400, 300);
  view.setSize(800, 600);

  uint32_t width = 400;
  uint32_t minHeight = view.getSize().y / 2;
  uint32_t pos = 0;

  sf::Event event;

  sf::ContextSettings contextSettings;
  contextSettings.stencilBits = 0;
  contextSettings.antialiasingLevel = 4;

  sf::RenderWindow window(sf::VideoMode(800, 600), "Progress", sf::Style::Default);
  window.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
  window.setVerticalSyncEnabled(true);

  bool redraw = true;
  while(window.isOpen()) {
    while(window.pollEvent(event)) {
      switch(event.type) {
        case sf::Event::Closed:
        {
          window.close();
          break;
        }
        case sf::Event::Resized:
        {
          view.setSize(static_cast<float>(event.size.width), static_cast<float>(event.size.height));
          width = event.size.width / 2;
          view.setCenter(width, view.getCenter().y);
          minHeight = view.getSize().y / 2;
          redraw = true;
          break;
        }
        case sf::Event::MouseWheelScrolled:
        {
          event.mouseWheelScroll.delta *= 20;
          pos = view.getCenter().y;
          view.setCenter(width, pos - event.mouseWheelScroll.delta);
          if(pos - event.mouseWheelScroll.delta < minHeight) {
            view.setCenter(width, minHeight);
          }
          redraw = true;
          break;
        }
        case sf::Event::KeyPressed:
        {
          if(event.key.code == sf::Keyboard::Up) {
            if(pos - 10 < minHeight) {
              view.setCenter(width, minHeight);
            }
            else {
              view.setCenter(width, view.getCenter().y - 10);
            }
          }
          if(event.key.code == sf::Keyboard::Down) {
            if(pos + 10 < minHeight) {
              view.setCenter(width, minHeight);
            }
            else {
              view.setCenter(width, view.getCenter().y + 10);
            }
          }
          redraw = true;
          break;
        }
      }
      redraw = tree.event(event, window) || redraw;
#ifdef DEBUG
      reallocStat();
#endif // DEBUG
    }

    if(redraw) {
      redraw = false;
      window.clear(sf::Color(0, 0, 128));
      window.setView(view);
      tree.draw(window);
      window.display();
    }
    else {
      sf::sleep(sf::milliseconds(15));
    }
  }
  save(tree);
}