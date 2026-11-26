#include <Geode/Geode.hpp>
#include <Geode/modify/CreatorLayer.hpp>
#include <set>
#include "levels.hpp"

using namespace geode::prelude;

class WorldLvl : public LevelDownloadDelegate {
public:
    WorldLevel wlvl;
    void levelDownloadFinished(GJGameLevel* level) override {
        GameLevelManager::get()->m_levelDownloadDelegate = nullptr;
        if (!level) return;
        level->m_creatorName = wlvl.authorName;
        level->m_accountID = wlvl.accountID;
        level->m_userID = wlvl.userID;
        auto scene = LevelInfoLayer::scene(level, false);
        scene->setUserObject("is-world-level"_spr, CCBool::create(true));
        CCDirector::get()->pushScene(CCTransitionFade::create(0.5f, scene));
    }
    void levelDownloadFailed(int id) override {
        GameLevelManager::get()->m_levelDownloadDelegate = nullptr;
        FLAlertLayer::create(
            "Download Failed",
            "Failed to download the level. Please try again later.", 
            "OK"
        )->show();
    }
};
static inline WorldLvl s_delegate;

class WorldsLayer : public cocos2d::CCLayer {
public:
    CREATE_FUNC(WorldsLayer);

    bool init() override {
        if (!CCLayer::init()) return false;
        auto win = CCDirector::get()->getWinSize();

        auto bg = CCSprite::create("World1.png"_spr);
        if (bg) {
            bg->setID("world-bg");
            bg->setAnchorPoint({0.5f, 0.5f});
            bg->setPosition({ win.width / 2, win.height / 2 });
            bg->setScaleX(win.width / bg->getContentSize().width);
            bg->setScaleY(win.height / bg->getContentSize().height);
            this->addChild(bg, -1);
        }

        auto menu = CCMenu::create();
        menu->setID("world-menu");
        menu->setPosition({0, 0});
        this->addChild(menu);

        auto infoSpr = CCSprite::createWithSpriteFrameName("GJ_infoBtn_001.png");
        infoSpr->setScale(0.8f);
        auto infoBtn = CCMenuItemSpriteExtra::create(infoSpr, nullptr, this, menu_selector(WorldsLayer::onInfo));
        infoBtn->setID("info-btn");
        infoBtn->setPosition({ win.width - infoSpr->getContentSize().width / 2 - 10.f,
                               win.height - infoSpr->getContentSize().height / 2 - 10.f });
        menu->addChild(infoBtn);

        auto discordSpr = CCSprite::createWithSpriteFrameName("gj_discordIcon_001.png");
        auto discordBtn = CCMenuItemSpriteExtra::create(discordSpr, nullptr, this, menu_selector(WorldsLayer::onDiscord));
        discordBtn->setID("discord-btn");
        discordBtn->setPosition({
            win.width - discordSpr->getContentSize().width / 2 - 13.f,
            discordSpr->getContentSize().height / 2 + 20.f
        });
        menu->addChild(discordBtn);

        auto title = CCLabelBMFont::create("W1 - Hello, World!", "bigFont.fnt");
        title->setID("world-name");
        title->setPosition({ win.width / 2, win.height - 25.f });
        title->setScale(0.6f);
        this->addChild(title);

        auto funnytext = CCLabelBMFont::create("This is where your adventure starts...", "bigFont.fnt");
        funnytext->setID("funny-text");
        funnytext->setPosition({ win.width / 2, win.height / 10 });
        funnytext->setScale(0.4f);
        this->addChild(funnytext);

        auto backSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
        auto backBtn = CCMenuItemSpriteExtra::create(backSpr, nullptr, this, menu_selector(WorldsLayer::onBack));
        backBtn->setPosition({30.f, win.height - 30.f});
        backBtn->setID("back-btn");
        menu->addChild(backBtn);

        this->refreshLvls(0);
        this->schedule(schedule_selector(WorldsLayer::refreshLvls), 1.0f);

        return true;
    }

    void refreshLvls(float) {
        auto win = CCDirector::get()->getWinSize();
        auto menu = static_cast<CCMenu*>(this->getChildByID("world-menu"));
        if (!menu) return;

        auto children = menu->getChildren();
        if (children && children->count() > 0) {
            for (int i = static_cast<int>(children->count()) - 1; i >= 0; --i) {
                auto btn = static_cast<CCNode*>(children->objectAtIndex(i));
                std::string id = btn->getID();
                if (id.rfind("lvl-", 0) == 0) {
                    menu->removeChild(btn, true);
                }
            }
        }

        auto lvls = getWorldLevels(win.width, win.height);
        std::set<int> completed;
        if (auto arr = GameLevelManager::get()->getCompletedLevels(false)) {
            for (auto g : CCArrayExt<GJGameLevel*>(arr)) {
                if (g) {
                    completed.insert(g->m_levelID.value());
                }
            }
        }

        for (size_t i = 0; i < lvls.size(); ++i) {
            const auto& e = lvls[i];
            bool unlocked = (e.order == 1) ||
                (e.order >= 2 && static_cast<size_t>(e.order - 2) < lvls.size() &&
                 completed.count(lvls[e.order - 2].levelID) > 0);
            const char* frame = unlocked
                ? "worldLevelBtn_001.png"_spr
                : "worldLevelBtn_locked_001.png"_spr;
            auto spr = CCSprite::createWithSpriteFrameName(frame);
            if (!spr) {
                spr = CCSprite::createWithSpriteFrameName("GJ_playBtn2_001.png");
            }
            if (i == lvls.size() - 1) {
                spr->setScale(1.5f);
            }
            auto btn = CCMenuItemSpriteExtra::create(spr, nullptr, this, menu_selector(WorldsLayer::onWorld));
            btn->setID("lvl-" + std::to_string(e.levelID));
            btn->setTag(e.levelID);
            btn->setPosition({e.x, e.y});
            btn->setEnabled(unlocked);
            menu->addChild(btn);
        }
    }

    void onDiscord(CCObject*) {
        web::openLinkInBrowser("https://dsc.gg");
    }

    void onInfo(CCObject*) {
        auto win = CCDirector::get()->getWinSize();

        if (this->getChildByID("help-me")) {
            this->onClosePromo(nullptr);
            return;
        }

        auto promo = CCSprite::create("promo.png"_spr);
        if (!promo) return;

        float scaleX = (win.width * 0.8f) / promo->getContentSize().width;
        float scaleY = (win.height * 0.8f) / promo->getContentSize().height;
        float scale = std::min(scaleX, scaleY);
        promo->setID("help-me");
        promo->setScale(scale);
        promo->setPosition({ win.width / 2, win.height / 2 });
        this->addChild(promo, 1000);

        float promoW = promo->getContentSize().width * scale;
        float promoH = promo->getContentSize().height * scale;

        auto xSpr = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
        auto closeBtn = CCMenuItemSpriteExtra::create(xSpr, nullptr, this, menu_selector(WorldsLayer::onClosePromo));
        closeBtn->setPosition({
            promo->getPositionX() - promoW / 2 + xSpr->getContentSize().width * 0.5f - 24.f,
            promo->getPositionY() + promoH / 2 - xSpr->getContentSize().height * 0.5f + 24.f
        });
        closeBtn->setID("promo-close");

        auto promoMenu = CCMenu::create();
        promoMenu->setID("promo-menu");
        promoMenu->setPosition({0, 0});
        promoMenu->addChild(closeBtn);
        this->addChild(promoMenu, 1001);
    }

    void onClosePromo(CCObject*) {
        if (auto promo = this->getChildByID("help-me"))
            promo->removeFromParentAndCleanup(true);
        if (auto promoMenu = this->getChildByID("promo-menu"))
            promoMenu->removeFromParentAndCleanup(true);
    }

    void onBack(CCObject*) {
        GameManager::get()->fadeInMenuMusic();
        CCDirector::get()->replaceScene(
            CCTransitionFade::create(0.5f, CreatorLayer::scene())
        );
    }

    void onWorld(CCObject* sender) {
        int id = static_cast<CCMenuItemSpriteExtra*>(sender)->getTag();
        auto win = CCDirector::get()->getWinSize();
        auto lvls = getWorldLevels(win.width, win.height);
        for (const auto& e : lvls) {
            if (e.levelID == id) {
                s_delegate.wlvl = e;
                break;
            }
        }
        auto glm = GameLevelManager::get();
        if (auto existing = glm->getSavedLevel(id)) {
            s_delegate.levelDownloadFinished(existing);
            return;
        }
        glm->m_levelDownloadDelegate = &s_delegate;
        glm->downloadLevel(id, false);
    }

    static CCScene* scene() {
        auto scene = CCScene::create();
        scene->addChild(WorldsLayer::create());
        return scene;
    }
};

class $modify(MyCreatorLayer, CreatorLayer) {
    bool init() {
        if (!CreatorLayer::init()) return false;

        if (auto menu = this->getChildByID("creator-buttons-menu")) {
            if (auto mapBtn = static_cast<CCMenuItemSpriteExtra*>(menu->getChildByID("the-map-button"))) {
                mapBtn->setTarget(this, menu_selector(MyCreatorLayer::onWorldsBtn));
                
                auto spr = CCSprite::create("worlds.png"_spr);
                if (spr) {
                    if (auto oldImg = mapBtn->getNormalImage()) {
                        float targetW = oldImg->getContentSize().width * oldImg->getScaleX();
                        spr->setScale(targetW / spr->getContentSize().width);
                    }
                    mapBtn->setNormalImage(spr);
                    mapBtn->updateSprite();
                }
                menu->updateLayout();
            }
        }
        return true;
    }

    void onWorldsBtn(CCObject*) {
        CCDirector::get()->replaceScene(CCTransitionFade::create(0.5f, WorldsLayer::scene()));
    }
};
