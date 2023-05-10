#states=('[1_hop_shortest]' '[1_hop_shortest,3x3_section]' '[1_hop_shortest,3x3_section,deflect_probability]' '[1_hop_shortest,3x3_section,drop_probability]' '[2_hop_shortest,1_hop_shortest]' '[2_hop_shortest,1_hop_shortest,3x3_section]' '[2_hop_shortest,1_hop_shortest,3x3_section,deflect_probability]' '[2_hop_shortest,1_hop_shortest,3x3_section,drop_probability]' '[dest_id]' '[dest_id,deflect_probability]' '[dest_id,drop_probability]')
#states=('[1_hop_shortest]' '[1_hop_shortest,3x3_section]' '[1_hop_shortest,3x3_section,deflect_probability]' '[1_hop_shortest,3x3_section,drop_probability]' '[2_hop_shortest,1_hop_shortest]' '[2_hop_shortest,1_hop_shortest,3x3_section]' '[2_hop_shortest,1_hop_shortest,3x3_section,deflect_probability]' '[2_hop_shortest,1_hop_shortest,3x3_section,drop_probability]')
states=('[1_hop_shortest,3x3_section]' '[2_hop_shortest,1_hop_shortest,3x3_section]')
#states=('[1_hop_shortest,3x3_section]' '[1_hop_shortest,3x3_section,deflect_probability]' '[1_hop_shortest,3x3_section,drop_probability]' '[2_hop_shortest,1_hop_shortest,3x3_section]' '[2_hop_shortest,1_hop_shortest,3x3_section,deflect_probability]' '[2_hop_shortest,1_hop_shortest,3x3_section,drop_probability]')
#states=('[dest_id]')
#states=('[2_hop_shortest,1_hop_shortest,3x3_section]' '[dest_id]')
#states=('[1_hop_shortest,3x3_section,drop_probability]' '[2_hop_shortest,1_hop_shortest,3x3_section,drop_probability]' '[dest_id,drop_probability]')
utils=(05 1 15 2)

#agents=('rand_deflect' 'rand_forward')
agents=()

for state in ${states[@]} ; do
    agents+=('mbd_'${state})
done

#5
n=8
network_size="${n}x${n}"

for agent in ${agents[@]} ; do
    for util in ${utils[@]}; do
        #bash run_test.sh -t 18000 -p 20 -n 20 configs/${network_size}/${agent}/fc_20/u_0.${util} ${network_size}_bursty_0.${util}_${agent}_fc_20 ${network_size}manhattan_flow_params.json
        echo bash run_test.sh -t 18000 -p 20 -n 20 configs/${network_size}/${agent}/u_0.${util} ${network_size}_bursty_0.${util}_${agent} ${network_size}manhattan_flow_params.json
    done
done
