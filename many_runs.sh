#states=('[1_hop_shortest]' '[1_hop_shortest,3x3_section]' '[1_hop_shortest,3x3_section,deflect_probability]' '[1_hop_shortest,3x3_section,drop_probability]' '[2_hop_shortest,1_hop_shortest]' '[2_hop_shortest,1_hop_shortest,3x3_section]' '[2_hop_shortest,1_hop_shortest,3x3_section,deflect_probability]' '[2_hop_shortest,1_hop_shortest,3x3_section,drop_probability]' '[dest_id]' '[dest_id,deflect_probability]' '[dest_id,drop_probability]')
#states=('[1_hop_shortest]' '[1_hop_shortest,3x3_section]' '[2_hop_shortest,1_hop_shortest]' '[2_hop_shortest,1_hop_shortest,3x3_section]' '[1-2_hop_shortest]')
#states=('[1_hop_shortest]' '[1_hop_shortest,3x3_section]' '[1_hop_shortest,3x3_section,deflect_probability]' '[1_hop_shortest,3x3_section,drop_probability]' '[2_hop_shortest,1_hop_shortest]' '[2_hop_shortest,1_hop_shortest,3x3_section]' '[2_hop_shortest,1_hop_shortest,3x3_section,deflect_probability]' '[2_hop_shortest,1_hop_shortest,3x3_section,drop_probability]')
#states=('[1_hop_shortest,3x3_section]' '[2_hop_shortest,1_hop_shortest,3x3_section]')

#states=('[1_hop_shortest,3x3_section]')

#states=('[1_hop_shortest]' '[1_hop_shortest,3x3_section]' '[2_hop_shortest,1_hop_shortest]' '[2_hop_shortest,1_hop_shortest,3x3_section]' '[1-2_hop_shortest]' '[1-2_hop_shortest,3x3_section]')

#states=('[1_hop_shortest,3x3_section]' '[1_hop_shortest,3x3_section,deflect_probability]' '[1_hop_shortest,3x3_section,drop_probability]' '[2_hop_shortest,1_hop_shortest,3x3_section]' '[2_hop_shortest,1_hop_shortest,3x3_section,deflect_probability]' '[2_hop_shortest,1_hop_shortest,3x3_section,drop_probability]')
#states=('[dest_id]')
#states=('[2_hop_shortest,1_hop_shortest,3x3_section]' '[dest_id]')
#states=('[1_hop_shortest,3x3_section,drop_probability]' '[2_hop_shortest,1_hop_shortest,3x3_section,drop_probability]' '[dest_id,drop_probability]')
#states=()
#utils=(05 1 15 2)
utils=(05)
props=(0.1)
#props=(0.01 0.1 0.5)

#agents=()
#agents=("rand_deflect" "rand_forward")
agents=("rand_forward")

for state in ${states[@]} ; do
    #agents+=('mbd_original_'${state})
    agents+=('mbd_slide_'${state})
    #agents+=('mbd_D-LinUCB_0.999_'${state})
    #agents+=('mbd_D-LinUCB_0.9999_'${state})
done

n=8
network_size="${n}x${n}"
parallel=30

for util in ${utils[@]}; do
    for agent in ${agents[@]} ; do
        for prop in ${props[@]} ; do
            #18000
            #bash run_test.sh -t 50000 -p $parallel -n $parallel configs/${network_size}/${agent}_prop_${prop}/u_0.${util} ${network_size}_bursty_0.${util}_${agent}_prop_${prop} ${network_size}manhattan_flow_params.json
            bash run_test.sh -t 50000 -p $parallel -n $parallel configs/${network_size}/${agent}_prop_${prop}/u_0.${util} ${network_size}_bursty_0.${util}_delete_prop_${prop} ${network_size}manhattan_flow_params.json
        done
    done
done
