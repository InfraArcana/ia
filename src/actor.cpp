#include "actor.h"

#include "init.h"

#include "render.h"
#include "game_time.h"
#include "actor_player.h"
#include "actor_mon.h"
#include "map.h"
#include "fov.h"
#include "log.h"
#include "feature_trap.h"
#include "drop.h"
#include "explosion.h"
#include "dungeon_master.h"
#include "inventory.h"
#include "map_parsing.h"
#include "item.h"
#include "utils.h"
#include "input.h"
#include "marker.h"
#include "look.h"

using namespace std;

Actor::Actor() :
    pos           (),
    state_        (Actor_state::alive),
    clr_          (clr_black),
    glyph_        (' '),
    tile_         (Tile_id::empty),
    hp_           (-1),
    hp_max_        (-1),
    spi_          (-1),
    spi_max_       (-1),
    lair_cell_     (),
    prop_handler_  (nullptr),
    data_         (nullptr),
    inv_          (nullptr) {}

Actor::~Actor()
{
    delete prop_handler_;
    delete inv_;
}

bool Actor::is_spotting_hidden_actor(Actor& other)
{
    const Pos& other_pos = other.pos;

    const int PLAYER_SEARCH_MOD =
        is_player() ?
        (data_->ability_vals.get_val(Ability_id::searching, true, *this) / 3) : 0;

    const auto& abilities_other  = other.get_data().ability_vals;

    const int   SNEAK_SKILL     = abilities_other.get_val(Ability_id::stealth, true, other);

    const int   DIST            = Utils::king_dist(pos, other_pos);
    const int   SNEAK_DIST_MOD  = get_constr_in_range(0, (DIST - 1) * 10, 60);
    const Cell& cell            = Map::cells[other_pos.x][other_pos.y];
    const int   SNEAK_LGT_MOD   = cell.is_lit                    ? -40 : 0;
    const int   SNEAK_DRK_MOD   = (cell.is_dark && ! cell.is_lit) ?  40 : 0;
    const int   SNEAK_TOT       = get_constr_in_range(
                                      0,
                                      SNEAK_SKILL     +
                                      SNEAK_DIST_MOD  +
                                      SNEAK_LGT_MOD   +
                                      SNEAK_DRK_MOD   -
                                      PLAYER_SEARCH_MOD,
                                      99);

    return Ability_roll::roll(SNEAK_TOT) <= fail_small;
}

int Actor::get_hp_max(const bool WITH_MODIFIERS) const
{
    return WITH_MODIFIERS ? prop_handler_->get_changed_max_hp(hp_max_) : hp_max_;
}

Actor_speed Actor::get_speed() const
{
    const auto base_speed = data_->speed;

    bool props[size_t(Prop_id::END)];

    prop_handler_->get_prop_ids(props);

    int speed_int = int(base_speed);

    //"Slowed" gives speed penalty
    if (props[int(Prop_id::slowed)] && speed_int > 0)
    {
        --speed_int;
    }

    //"Hasted" or "frenzied" gives speed bonus.
    if (
        (props[int(Prop_id::hasted)] || props[int(Prop_id::frenzied)]) &&
        speed_int < int(Actor_speed::END) - 1)
    {
        ++speed_int;
    }

    assert(speed_int >= 0 && speed_int < int(Actor_speed::END));

    return Actor_speed(speed_int);
}

bool Actor::can_see_actor(const Actor& other, const bool blocked_los[MAP_W][MAP_H]) const
{
    if (this == &other)
    {
        return true;
    }

    if (!other.is_alive())
    {
        return false;
    }

    if (is_player())
    {
        return Map::cells[other.pos.x][other.pos.y].is_seen_by_player &&
               !static_cast<const Mon*>(&other)->is_stealth_;
    }

    //This point reached means its a monster checking

    if (
        pos.x - other.pos.x > FOV_STD_RADI_INT ||
        other.pos.x - pos.x > FOV_STD_RADI_INT ||
        other.pos.y - pos.y > FOV_STD_RADI_INT ||
        pos.y - other.pos.y > FOV_STD_RADI_INT)
    {
        return false;
    }

    //Monster allied to player looking at other monster?
    if (
        is_actor_my_leader(Map::player)  &&
        !other.is_player()             &&
        static_cast<const Mon*>(&other)->is_stealth_)
    {
        return false;
    }

    if (!prop_handler_->allow_see())
    {
        return false;
    }

    if (blocked_los)
    {
        return Fov::check_cell(blocked_los, other.pos, pos, !data_->can_see_in_darkness);
    }

    return false;
}

void Actor::get_seen_foes(vector<Actor*>& out)
{
    out.clear();

    bool blocked_los[MAP_W][MAP_H];

    if (!is_player())
    {
        Rect los_rect(max(0,         pos.x - FOV_STD_RADI_INT),
                     max(0,         pos.y - FOV_STD_RADI_INT),
                     min(MAP_W - 1, pos.x + FOV_STD_RADI_INT),
                     min(MAP_H - 1, pos.y + FOV_STD_RADI_INT));

        Map_parse::run(Cell_check::Blocks_los(), blocked_los, Map_parse_mode::overwrite,
                      los_rect);
    }

    for (Actor* actor : Game_time::actors_)
    {
        if (actor != this && actor->is_alive())
        {
            if (is_player())
            {
                if (can_see_actor(*actor, nullptr) && !is_leader_of(actor))
                {
                    out.push_back(actor);
                }
            }
            else //Not player
            {
                const bool IS_HOSTILE_TO_PLAYER = !is_actor_my_leader(Map::player);
                const bool IS_OTHER_HOSTILE_TO_PLAYER =
                    actor->is_player() ? false : !actor->is_actor_my_leader(Map::player);

                //"IS_OTHER_HOSTILE_TO_PLAYER" is false if other IS the player, there is
                //no need to check if "IS_HOSTILE_TO_PLAYER && IS_OTHER_PLAYER"
                if (
                    (IS_HOSTILE_TO_PLAYER  && !IS_OTHER_HOSTILE_TO_PLAYER) ||
                    (!IS_HOSTILE_TO_PLAYER &&  IS_OTHER_HOSTILE_TO_PLAYER))
                {
                    if (can_see_actor(*actor, blocked_los))
                    {
                        out.push_back(actor);
                    }
                }
            }
        }
    }
}

void Actor::place(const Pos& pos_, Actor_data_t& data)
{
    pos             = pos_;
    data_           = &data;
    inv_            = new Inventory();
    prop_handler_    = new Prop_handler(this);
    state_          = Actor_state::alive;
    clr_            = data_->color;
    glyph_          = data_->glyph;
    tile_           = data_->tile;
    hp_             = hp_max_  = data_->hp;
    spi_            = spi_max_ = data_->spi;
    lair_cell_       = pos;

    if (data_->id != Actor_id::player) {mk_start_items();}

    place_();

    update_clr();
}

void Actor::teleport()
{
    bool blocked[MAP_W][MAP_H];
    Map_parse::run(Cell_check::Blocks_actor(*this, true), blocked);

    vector<Pos> pos_bucket;
    Utils::mk_vector_from_bool_map(false, blocked, pos_bucket);

    if (pos_bucket.empty())
    {
        return;
    }

    if (!is_player() && Map::player->can_see_actor(*this, nullptr))
    {
        Log::add_msg(get_name_the() + " suddenly disappears!");
    }

    Pos   tgt_pos                = pos_bucket[Rnd::range(0, pos_bucket.size() - 1)];
    bool  player_has_tele_control  = false;

    if (is_player())
    {
        Map::player->update_fov();
        Render::draw_map_and_interface();
        Map::update_visual_memory();

        //Teleport control?
        bool props[size_t(Prop_id::END)];

        prop_handler_->get_prop_ids(props);

        if (props[int(Prop_id::tele_ctrl)] && !props[int(Prop_id::confused)])
        {
            player_has_tele_control = true;

            auto get_chance_of_tele_success = [](const Pos & tgt)
            {
                const int DIST = Utils::king_dist(Map::player->pos, tgt);
                return get_constr_in_range(25, 100 - DIST, 95);
            };

            auto on_marker_at_pos = [get_chance_of_tele_success](const Pos & p)
            {
                Log::clear_log();
                Look::print_location_info_msgs(p);

                const int CHANCE_PCT = get_chance_of_tele_success(p);

                Log::add_msg(to_str(CHANCE_PCT) + "% chance of success.");

                Log::add_msg("[enter] to teleport here");
                Log::add_msg(cancel_info_str_no_space);
            };

            auto on_key_press = [](const Pos & p, const Key_data & key_data)
            {
                (void)p;

                if (key_data.sdl_key == SDLK_RETURN)
                {
                    Log::clear_log();
                    return Marker_done::yes;
                }
                return Marker_done::no;
            };

            Log::add_msg("I have the power to control teleportation.", clr_white, false,
                        true);

            const Pos marker_tgt_pos =
                Marker::run(Marker_draw_tail::yes, Marker_use_player_tgt::no, on_marker_at_pos,
                            on_key_press);

            if (blocked[marker_tgt_pos.x][marker_tgt_pos.y])
            {
                //Blocked
                Log::add_msg("Something is blocking me...", clr_white, false, true);
            }
            else if (Rnd::percent(get_chance_of_tele_success(marker_tgt_pos)))
            {
                //Success
                tgt_pos = marker_tgt_pos;
            }
            else //Distance roll failed
            {
                Log::add_msg("I failed to go there...", clr_white, false, true);
            }
        }
    }
    else
    {
        static_cast<Mon*>(this)->player_aware_of_me_counter_ = 0;
    }

    pos = tgt_pos;

    if (is_player())
    {
        Map::player->update_fov();
        Render::draw_map_and_interface();
        Map::update_visual_memory();
        if (!player_has_tele_control)
        {
            Log::add_msg("I suddenly find myself in a different location!");
            prop_handler_->try_apply_prop(new Prop_confused(Prop_turns::specific, 8));
        }
    }
}

void Actor::update_clr()
{
    if (state_ != Actor_state::alive)
    {
        clr_ = data_->color;
        return;
    }

    if (prop_handler_->change_actor_clr(clr_))
    {
        return;
    }

    if (is_player() && Map::player->active_explosive)
    {
        clr_ = clr_yellow;
        return;
    }

    clr_ = data_->color;
}

bool Actor::restore_hp(const int HP_RESTORED, const bool ALLOW_MSG,
                      const bool IS_ALLOWED_ABOVE_MAX)
{
    bool      is_hp_gained    = IS_ALLOWED_ABOVE_MAX;
    const int DIF_FROM_MAX  = get_hp_max(true) - HP_RESTORED;

    //If hp is below limit, but restored hp will push it over the limit, HP is set to max.
    if (!IS_ALLOWED_ABOVE_MAX && get_hp() > DIF_FROM_MAX && get_hp() < get_hp_max(true))
    {
        hp_         = get_hp_max(true);
        is_hp_gained  = true;
    }

    //If HP is below limit, and restored hp will NOT push it over the limit -
    //restored hp is added to current.
    if (IS_ALLOWED_ABOVE_MAX || get_hp() <= DIF_FROM_MAX)
    {
        hp_ += HP_RESTORED;
        is_hp_gained = true;
    }

    update_clr();

    if (ALLOW_MSG && is_hp_gained)
    {
        if (is_player())
        {
            Log::add_msg("I feel healthier!", clr_msg_good);
        }
        else //Is a monster
        {
            if (Map::player->can_see_actor(*this, nullptr))
            {
                Log::add_msg(data_->name_the + " looks healthier.");
            }
        }
        Render::draw_map_and_interface();
    }

    return is_hp_gained;
}

bool Actor::restore_spi(const int SPI_RESTORED, const bool ALLOW_MSG,
                       const bool IS_ALLOWED_ABOVE_MAX)
{
    bool is_spi_gained = IS_ALLOWED_ABOVE_MAX;

    const int DIF_FROM_MAX = get_spi_max() - SPI_RESTORED;

    //If spi is below limit, but will be pushed over the limit, spi is set to max.
    if (!IS_ALLOWED_ABOVE_MAX && get_spi() > DIF_FROM_MAX && get_spi() < get_spi_max())
    {
        spi_        = get_spi_max();
        is_spi_gained = true;
    }

    //If spi is below limit, and will not NOT be pushed over the limit - restored spi is
    //added to current.
    if (IS_ALLOWED_ABOVE_MAX || get_spi() <= DIF_FROM_MAX)
    {
        spi_ += SPI_RESTORED;
        is_spi_gained = true;
    }

    if (ALLOW_MSG && is_spi_gained)
    {
        if (is_player())
        {
            Log::add_msg("I feel more spirited!", clr_msg_good);
        }
        else
        {
            if (Map::player->can_see_actor(*this, nullptr))
            {
                Log::add_msg(data_->name_the + " looks more spirited.");
            }
        }
        Render::draw_map_and_interface();
    }

    return is_spi_gained;
}

void Actor::change_max_hp(const int CHANGE, const bool ALLOW_MSG)
{
    hp_max_  = max(1, hp_max_ + CHANGE);
    hp_     = max(1, hp_ + CHANGE);

    if (ALLOW_MSG)
    {
        if (is_player())
        {
            if (CHANGE > 0)
            {
                Log::add_msg("I feel more vigorous!", clr_msg_good);
            }
            else if (CHANGE < 0)
            {
                Log::add_msg("I feel frailer!", clr_msg_bad);
            }
        }
        else //Is monster
        {
            if (Map::player->can_see_actor(*this, nullptr))
            {
                if (CHANGE > 0)
                {
                    Log::add_msg(get_name_the() + " looks more vigorous.");
                }
                else if (CHANGE < 0)
                {
                    Log::add_msg(get_name_the() + " looks frailer.");
                }
            }
        }
    }
}

void Actor::change_max_spi(const int CHANGE, const bool ALLOW_MSG)
{
    spi_max_ = max(1, spi_max_ + CHANGE);
    spi_    = max(1, spi_ + CHANGE);

    if (ALLOW_MSG)
    {
        if (is_player())
        {
            if (CHANGE > 0)
            {
                Log::add_msg("My spirit is stronger!", clr_msg_good);
            }
            else if (CHANGE < 0)
            {
                Log::add_msg("My spirit is weaker!", clr_msg_bad);
            }
        }
        else //Is monster
        {
            if (Map::player->can_see_actor(*this, nullptr))
            {
                if (CHANGE > 0)
                {
                    Log::add_msg(get_name_the() + " appears to grow in spirit.");
                }
                else if (CHANGE < 0)
                {
                    Log::add_msg(get_name_the() + " appears to shrink in spirit.");
                }
            }
        }
    }
}

Actor_died Actor::hit(int dmg, const Dmg_type dmg_type, Dmg_method method)
{
    TRACE_FUNC_BEGIN_VERBOSE;

    if (state_ == Actor_state::destroyed)
    {
        TRACE_FUNC_END_VERBOSE;
        return Actor_died::no;
    }

    TRACE_VERBOSE << "Damage from parameter: " << dmg << endl;

    bool props[size_t(Prop_id::END)];
    prop_handler_->get_prop_ids(props);

    if (dmg_type == Dmg_type::light && !props[int(Prop_id::lgt_sens)])
    {
        return Actor_died::no;
    }

    if (is_player()) {Map::player->interrupt_actions();}

    //Damage to corpses
    //NOTE: Corpse is automatically destroyed if damage is high enough, otherwise it is
    //destroyed with a random chance
    if (is_corpse() && !is_player())
    {
        if (Rnd::fraction(5, 8) || dmg >= ((get_hp_max(true) * 2) / 3))
        {
            if (method == Dmg_method::kick)
            {
                Snd_emit::emit_snd({"*Crack!*", Sfx_id::hit_corpse_break, Ignore_msg_if_origin_seen::yes,
                                  pos, nullptr, Snd_vol::low, Alerts_mon::yes
                                 });
            }

            state_ = Actor_state::destroyed;
            glyph_ = ' ';

            if (is_humanoid()) {Map::mk_gore(pos);}

            if (Map::cells[pos.x][pos.y].is_seen_by_player)
            {
                Log::add_msg(get_corpse_name_the() + " is destroyed.");
            }
        }
        else //Not destroyed
        {
            if (method == Dmg_method::kick)
            {
                Snd_emit::emit_snd({"*Thud*", Sfx_id::hit_medium, Ignore_msg_if_origin_seen::yes, pos,
                                  nullptr, Snd_vol::low, Alerts_mon::yes
                                 });
            }
        }
        TRACE_FUNC_END_VERBOSE;
        return Actor_died::no;
    }

    if (dmg_type == Dmg_type::spirit) {return hit_spi(dmg, true);}

    //Property resists?
    const bool ALLOW_DMG_RES_MSG = is_alive();
    if (prop_handler_->try_resist_dmg(dmg_type, ALLOW_DMG_RES_MSG))
    {
        TRACE_FUNC_END_VERBOSE;
        return Actor_died::no;
    }

    on_hit(dmg);
    TRACE_VERBOSE << "Damage after on_hit(): " << dmg << endl;

    dmg = max(1, dmg);

    //Filter damage through worn armor
    if (is_humanoid())
    {
        Armor* armor = static_cast<Armor*>(inv_->get_item_in_slot(Slot_id::body));
        if (armor)
        {
            TRACE_VERBOSE << "Has armor, running hit on armor" << endl;

            if (dmg_type == Dmg_type::physical)
            {
                dmg = armor->take_dur_hit_and_get_reduced_dmg(dmg);

                if (armor->is_destroyed())
                {
                    TRACE << "Armor was destroyed" << endl;
                    if (is_player())
                    {
                        const string armor_name =
                            armor->get_name(Item_ref_type::plain, Item_ref_inf::none);
                        Log::add_msg("My " + armor_name + " is torn apart!", clr_msg_note);
                    }
                    delete armor;
                    armor = nullptr;
                    inv_->slots_[int(Slot_id::body)].item = nullptr;
                }
            }
        }
    }

    prop_handler_->on_hit();

    if (!is_player() || !Config::is_bot_playing()) {hp_ -= dmg;}

    if (get_hp() <= 0)
    {
        const bool IS_ON_BOTTOMLESS = Map::cells[pos.x][pos.y].rigid->is_bottomless();
        const bool IS_DMG_ENOUGH_TO_DESTROY = dmg > ((get_hp_max(true) * 5) / 4);
        const bool IS_DESTROYED = !data_->can_leave_corpse  ||
                                  IS_ON_BOTTOMLESS        ||
                                  IS_DMG_ENOUGH_TO_DESTROY;

        die(IS_DESTROYED, !IS_ON_BOTTOMLESS, !IS_ON_BOTTOMLESS);
        TRACE_FUNC_END_VERBOSE;
        return Actor_died::yes;
    }
    else //HP is greater than 0
    {
        TRACE_FUNC_END_VERBOSE;
        return Actor_died::no;
    }
}

Actor_died Actor::hit_spi(const int DMG, const bool ALLOW_MSG)
{
    if (ALLOW_MSG)
    {
        if (is_player())
        {
            Log::add_msg("My spirit is drained!", clr_msg_bad);
        }
    }

    prop_handler_->on_hit();

    if (!is_player() || !Config::is_bot_playing())
    {
        spi_ = max(0, spi_ - DMG);
    }
    if (get_spi() <= 0)
    {
        if (is_player())
        {
            Log::add_msg("All my spirit is depleted, I am devoid of life!", clr_msg_bad);
        }
        else
        {
            if (Map::player->can_see_actor(*this, nullptr))
            {
                Log::add_msg(get_name_the() + " has no spirit left!");
            }
        }

        const bool IS_ON_BOTTOMLESS = Map::cells[pos.x][pos.y].rigid->is_bottomless();
        const bool IS_DESTROYED     = !data_->can_leave_corpse || IS_ON_BOTTOMLESS;

        die(IS_DESTROYED, false, true);
        return Actor_died::yes;
    }
    return Actor_died::no;
}

void Actor::die(const bool IS_DESTROYED, const bool ALLOW_GORE,
                const bool ALLOW_DROP_ITEMS)
{
    assert(data_->can_leave_corpse || IS_DESTROYED);

    //Check all monsters and unset this actor as leader
    for (Actor* actor : Game_time::actors_)
    {
        if (actor != this && !actor->is_player() && is_leader_of(actor))
        {
            static_cast<Mon*>(actor)->leader_ = nullptr;
        }
    }

    bool is_on_visible_trap = false;

    //If died on a visible trap, destroy the corpse
    const auto* const f = Map::cells[pos.x][pos.y].rigid;
    if (f->get_id() == Feature_id::trap)
    {
        if (!static_cast<const Trap*>(f)->is_hidden()) {is_on_visible_trap = true;}
    }

    bool is_player_see_dying_actor = true;

    if (!is_player())
    {
        //If this monster is player's target, unset the target
        if (Map::player->tgt_ == this)
        {
            Map::player->tgt_ = nullptr;
        }

        //Print death messages
        if (Map::player->can_see_actor(*this, nullptr))
        {
            is_player_see_dying_actor = true;

            const string& death_msg_override = data_->death_msg_override;

            if (!death_msg_override.empty())
            {
                Log::add_msg(death_msg_override);
            }
            else
            {
                Log::add_msg(get_name_the() + " dies.");
            }
        }
    }

    if (IS_DESTROYED || (is_on_visible_trap && !is_player()))
    {
        state_ = Actor_state::destroyed;
    }
    else
    {
        state_ = Actor_state::corpse;
    }

    if (!is_player())
    {
        if (is_humanoid())
        {
            Snd_emit::emit_snd({"I hear agonized screaming.", Sfx_id::END,
                              Ignore_msg_if_origin_seen::yes, pos, this, Snd_vol::low,
                              Alerts_mon::no
                             });
        }
    }

    if (ALLOW_DROP_ITEMS)
    {
        Item_drop::drop_all_characters_items(*this);
    }

    if (IS_DESTROYED)
    {
        glyph_ = ' ';
        tile_ = Tile_id::empty;
        if (is_humanoid() && ALLOW_GORE)
        {
            Map::mk_gore(pos);
        }
    }
    else //Not destroyed
    {
        if (!is_player())
        {
            Pos new_pos;
            auto* feature_here = Map::cells[pos.x][pos.y].rigid;
            //TODO: this should be decided with a floodfill instead
            if (!feature_here->can_have_corpse())
            {
                for (int dx = -1; dx <= 1; ++dx)
                {
                    for (int dy = -1; dy <= 1; ++dy)
                    {
                        new_pos      = pos + Pos(dx, dy);
                        feature_here = Map::cells[pos.x + dx][pos.y + dy].rigid;
                        if (feature_here->can_have_corpse())
                        {
                            pos.set(new_pos);
                            dx = 9999;
                            dy = 9999;
                        }
                    }
                }
            }
        }
        glyph_ = '&';
        tile_ = Tile_id::corpse2;
    }

    clr_ = clr_red_lgt;

    on_death();

    prop_handler_->on_death(is_player_see_dying_actor);

    if (!is_player())
    {
        Dungeon_master::on_mon_killed(*this);
        static_cast<Mon*>(this)->leader_ = nullptr;
    }

    Render::draw_map_and_interface();
}

void Actor::add_light(bool light_map[MAP_W][MAP_H]) const
{
    bool props[size_t(Prop_id::END)];
    prop_handler_->get_prop_ids(props);

    if (state_ == Actor_state::alive && props[int(Prop_id::radiant)])
    {
        //TODO: Much of the code below is duplicated from Actor_player::add_light_(), some
        //refactoring is needed.

        bool my_light[MAP_W][MAP_H];
        Utils::reset_array(my_light, false);
        const int RADI = FOV_STD_RADI_INT;
        Pos p0(max(0, pos.x - RADI), max(0, pos.y - RADI));
        Pos p1(min(MAP_W - 1, pos.x + RADI), min(MAP_H - 1, pos.y + RADI));

        bool blocked_los[MAP_W][MAP_H];
        for (int y = p0.y; y <= p1.y; ++y)
        {
            for (int x = p0.x; x <= p1.x; ++x)
            {
                const auto* const f = Map::cells[x][y].rigid;
                blocked_los[x][y]    = !f->is_los_passable();
            }
        }

        Fov::run_fov_on_array(blocked_los, pos, my_light, false);
        for (int y = p0.y; y <= p1.y; ++y)
        {
            for (int x = p0.x; x <= p1.x; ++x)
            {
                if (my_light[x][y])
                {
                    light_map[x][y] = true;
                }
            }
        }
    }
    else if (props[int(Prop_id::burning)])
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            for (int dy = -1; dy <= 1; ++dy)
            {
                light_map[pos.x + dx][pos.y + dy] = true;
            }
        }
    }
    add_light_(light_map);
}

bool Actor::is_player() const
{
    return this == Map::player;
}