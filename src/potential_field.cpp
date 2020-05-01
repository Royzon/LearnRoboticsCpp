#include "potential_field.hpp"


PotentialField::PotentialField(
            const std::vector<float>& ox, const std::vector<float>& oy,
            float reso, float robot_radius, 
            float kp, float eta, float area_width) : 
            ox_(ox), oy_(oy), reso_(reso), rr_(robot_radius), 
            kp_(kp), eta_(eta), area_width_(area_width) {

        // figure out bounds
        minx_ = *std::min_element(ox.begin(), ox.end()) - area_width_/2.0f;
        miny_ = *std::min_element(oy.begin(), oy.end()) - area_width_/2.0f;
        maxx_ = *std::max_element(ox.begin(), ox.end()) + area_width_/2.0f;
        maxy_ = *std::max_element(oy.begin(), oy.end()) + area_width_/2.0f;
}



float PotentialField::calculateAttractivePotential(
    float x, float y, float gx, float gy) {
    return 0.5f * kp_ * std::hypot(x - gx, y - gy);
}

float PotentialField::calculateRepulsivePotential(float x, float y) {
    // find closest obs
    float min_dist = std::numeric_limits<float>::max();
    for (int i = 0; i < ox_.size(); ++i) {
        float d = std::hypot(x - ox_[i], y - oy_[i] );
        if ( d <= min_dist) {
            min_dist = d;
        }
    }

    float dq = min_dist;

    if (dq <= rr_) {
        if (dq <= 0.1) 
            dq = 0.1;
        return 0.5 * eta_ * std::pow((1.0 / dq - 1.0 / rr_), 2);
    } else {
        return 0.0;
    }
}



void PotentialField::generatePotentialMap(float gx, float gy) {
    int xw = std::round( (maxx_ - minx_) / reso_  );
    int yw = std::round( (maxy_ - miny_) / reso_  );

    // generate pmap
    pmap_ = std::vector<std::vector<float>>(xw, std::vector<float>(yw, 0.f));
    for (int ix = 0; ix < xw; ++ix) {
        float x = ix * reso_ + minx_;
        for (int iy = 0; iy < yw; ++iy) {
            float y = iy * reso_ + miny_;
            float ug = calculateAttractivePotential(x, y, gx, gy);
            float uo = calculateRepulsivePotential(x, y);
            float uf = uo + ug;
            pmap_[ix][iy] = uf;
        }
    }

    // visualize pmap in the future
    // viz_pmap_.clear();
    // viz_pmap_.reserve(xw * yw);
    // for (int ix = 0; ix < pmap_.size(); ++ix) {
    //     float x = ix * reso_ + minx_;
    //     for (int iy = 0; iy < pmap_.size(); ++iy)  {
    //         float y = iy * reso_ + miny_;
    //         std::cout << pmap_[ix][iy]   << " / " << pot_sum << std::endl;
    //         viz_pmap_.emplace_back(x, y, pmap_[ix][iy] / pot_sum);
    //     }
    // }
}


Path PotentialField::plan(float sx, float sy, float gx, float gy, Gnuplot& gp) {
    
    generatePotentialMap(gx, gy);

    float d = std::hypot(sx - gx, sy - gy);
    int ix = std::round((sx - minx_) / reso_);
    int iy = std::round((sy - miny_) / reso_);
    int gix = std::round((gx - minx_) / reso_);
    int giy = std::round((gy - miny_) / reso_);

    Path path;
    while (d >= reso_) {
        int minix = -1;
        int miniy = -1;
        float min_p = std::numeric_limits<float>::max();
        for (auto [dx, dy] : motion_model_) {
            int inx = ix + dx, iny = iy + dy;
            float p;
            // check out of bounds
            if (inx >= pmap_.size() || iny >= pmap_[0].size()) {
                p = std::numeric_limits<float>::max();
            } else {
                p = pmap_[inx][iny];
            }

            if (min_p > p) {
                min_p = p;
                minix = inx; miniy = iny;
            }
        }

        ix = minix; iy = miniy;
        float xp = ix * reso_ + minx_;
        float yp = iy * reso_ + miny_;
        d = std::hypot(gx - xp, gy - yp);
        path.emplace_back(xp, yp);


        gp << "plot '-' title 'path',"
            "'-' title 'goal',"
            "'-' title 'obs' pointsize 4 pointtype 7\n";
        gp.send1d(path);
        std::vector<std::tuple<float, float>> goal_plot = { {gx, gy}};
        gp.send1d(goal_plot);
        gp.send1d(boost::make_tuple(ox_, oy_));


    }


    return path;
}






int main(int argc, char *argv[])
{
    float sx = 0.0f;
    float sy = 10.0f;
    float gx = 30.0f;
    float gy = 30.0f;

    float grid_size = 0.5f;
    float robot_radius = 5.0f;

    std::vector<float> ox = {15.0f, 5.0f, 20.0f, 25.0f};
    std::vector<float> oy = {25.0f, 15.0f, 26.0f, 25.0f};

    Gnuplot gp;
    
    gp << "set size ratio 1.0\n";
    gp << "set xrange [0:40]\nset yrange [0:40]\n";
    gp << "set term gif animate\n";
    gp << "set output '../animations/potential_field.gif'\n";
    // gp << "set key left bottom\n";

    PotentialField pf(ox, oy, grid_size, robot_radius);
    Path path = pf.plan(sx, sy, gx, gy, gp);

    gp << "set output\n";


    return 0;
}